/*
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Tushar Krishna
 */

#include "cpu/testers/garnet_synthetic_traffic/GarnetSyntheticTraffic.hh"

#include <cmath>
#include <iomanip>
#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>  
#include "base/logging.hh"
#include "base/random.hh"
#include "base/statistics.hh"
#include "debug/GarnetSyntheticTraffic.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "mem/request.hh"
#include "sim/sim_events.hh"
#include "sim/stats.hh"
#include "sim/system.hh"
#include <unistd.h>

// CPU satus
# define IDLE               (int(0))
# define WORKIING           (int(1)) 
# define FINISH             (int(2)) 
// CPU working status
# define WORK_WAIT          (int(3))
# define WORK_CAL           (int(4))
# define WORK_SEND          (int(5))
# define WORK_IDLE          (int(6))

std::string all_status[7] =
{   "IDLE",
    "WORK",
    "FINISH",
    "WORK_WAIT",
    "WORK_CAL",
    "WORK_SEND",
    "WORK_IDLE" };

using namespace std;

int TESTER_NETWORK=0;

bool
GarnetSyntheticTraffic::CpuPort::recvTimingResp(PacketPtr pkt)
{
    tester->completeRequest(pkt);
    return true;
}

void
GarnetSyntheticTraffic::CpuPort::recvReqRetry()
{
    tester->doRetry();
}

void
GarnetSyntheticTraffic::sendPkt(PacketPtr pkt)
{
    if (!cachePort.sendTimingReq(pkt)) {
        retryPkt = pkt; // RubyPort will retry sending
    }
    numPacketsSent++;
}

// fanxitodo: change the vnet setting choice
GarnetSyntheticTraffic::GarnetSyntheticTraffic(const Params *p)
    : MemObject(p),
      tickEvent([this]{ tick(); }, "GarnetSyntheticTraffic tick",
                false, Event::CPU_Tick_Pri),
      cachePort("GarnetSyntheticTraffic", this),
      retryPkt(NULL),
      size(p->memory_size),
      blockSizeBits(p->block_offset),
      numDestinations(p->num_dest),
      simCycles(p->sim_cycles),
      numPacketsMax(p->num_packets_max),
      numPacketsSent(0),
      singleSender(p->single_sender),
      singleDest(p->single_dest),
      trafficType(p->traffic_type),
      injRate(p->inj_rate),
      injVnet(p->inj_vnet),
      if_routerless(p->if_routerless),
      if_debug(p->if_debug),
      dnn_task(p->dnn_task),
      precision(p->precision),
      responseLimit(p->response_limit),
      masterId(p->system->getMasterId(name()))
{
    // set up counters
    noResponseCycles = 0;
    schedule(tickEvent, 0);

    initTrafficType();
    if (trafficStringToEnum.count(trafficType) == 0) {
        fatal("Unknown Traffic Type: %s!\n", traffic);
    }
    traffic = trafficStringToEnum[trafficType];

    id = TESTER_NETWORK++;
    DPRINTF(GarnetSyntheticTraffic,"Config Created: Name = %s , and id = %d\n",
            name(), id);
}

BaseMasterPort &
GarnetSyntheticTraffic::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "test")
        return cachePort;
    else
        return MemObject::getMasterPort(if_name, idx);
}

void init_recv_packet_files(int id){
    std::string file;
	file = "./../recv/"+std::to_string(id)+".txt";
	ofstream OutFile(file);
	OutFile << std::to_string(0); 
    OutFile.close();    
}

void
GarnetSyntheticTraffic::init()
{   
    cpu_status = IDLE;
    cpu_work_stats = WORK_IDLE;
    numPacketsSent = 0;
    current_line_num = 0;
    init_recv_packet_files(id);

    // get current working path 
    char cwd[100];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
       std::cout <<"Current working dir:  "<< cwd << std::endl;
    } else {
       std::cout << "getcwd() error" << std::endl;
    }
}


void
GarnetSyntheticTraffic::completeRequest(PacketPtr pkt)
{   
    DPRINTF(GarnetSyntheticTraffic,
            "Completed injection of %s packet for address %x\n",
            pkt->isWrite() ? "write" : "read\n",
            pkt->req->getPaddr());

    assert(pkt->isResponse());
    noResponseCycles = 0;
    delete pkt;
}

int GarnetSyntheticTraffic::recv_packets(int id)
{
	std::string file;
	file = "./../run_info/node_recv/"+std::to_string(id)+".txt";
	ifstream infile; 
    infile.open(file.data());  
    assert(infile.is_open());   

    string s;
    while(getline(infile,s))
    {
        if(if_debug==1) 
            std::cout<<"node " << id <<" is recv_packets ing, packets="<< s <<std::endl;
    }
    infile.close();             //关闭文件输入流 
    return atoi(s.c_str());
}



// 1代表读到task 0 代表没有读到，文件结束
int GarnetSyntheticTraffic::get_task(int id,int line_num)
{
	std::string file;
	file = "./../dnn_task/"+dnn_task+"/"+std::to_string(id)+".txt";
	ifstream infile; 
    infile.open(file.data());  
    assert(infile.is_open());   

    int read_line_num=0;
    while(getline(infile,current_task_line))
    {
        if (read_line_num == line_num){
            break;
        }
        read_line_num += 1;
    }
    infile.close();             //关闭文件输入流 
    if (read_line_num < line_num || current_task_line == ""){ //文件最后会有个空行
        return 0;
    }
    else {
        if(if_debug==1) std::cout<<"node " << id <<" is get_task ing, linenum = " <<line_num << " the task = "<< current_task_line <<std::endl;
        return 1;   
    }

}

vector<string> split(const string &str, const string &pattern)
{
    vector<string> res;
    if(str == "")
        return res;
    //在字符串末尾也加入分隔符，方便截取最后一段
    string strs = str + pattern;
    size_t pos = strs.find(pattern);

    while(pos != strs.npos)
    {
        string temp = strs.substr(0, pos);
        res.push_back(temp);
        //去掉已分割的字符串,在剩下的字符串中进行分割
        strs = strs.substr(pos+1, strs.size());
        pos = strs.find(pattern);
    }

    return res;
}

// current not used 
void GarnetSyntheticTraffic::tell_mem_send_data(std::string src_mem_index,std::string num_wait_packets,int id) 
{
	std::string file;
	file = "./../dnn_task/"+dnn_task+"/"+src_mem_index+".txt";
    fstream f;

    std::string message_to_write = "send ";
    message_to_write.append(std::to_string(id));
    message_to_write.append(" ");
    message_to_write.append(num_wait_packets);
    //追加写入,在原来基础上加了ios::app 
	f.open(file,ios::out|ios::app);
    f<<message_to_write<<endl; 
    f.close(); 
    if(if_debug==1) std::cout<<"node "<< id << "is tell_mem_send_data ing" <<  std::endl;
}


void
GarnetSyntheticTraffic::tick()
{      
    bool sendAllowedThisCycle = false;
    if (traffic == DNN_) {
        if(if_debug==1) std::cout << "node "<<id<<"\tstatus: " << all_status[cpu_status] \
                                  << "\twork_stats: " << all_status[cpu_work_stats] \
                                  << "\tline_num:"<< current_line_num << "\t@"<< curTick() << std::endl;

        int if_get_task;

        // idle status : read task file
        if (cpu_status == IDLE){
            if_get_task = get_task(id, current_line_num);


            if (if_get_task == 0){
                cpu_status = IDLE;
            }

            else{// 解析task_line
                current_line_num += 1;
                vector<string>  current_task;
                current_task  = split(current_task_line," ");

                if (current_task[0] == "wait"){
                    if(if_debug==1) std::cout << "current_task == wait" << std::endl;
                    cpu_status = WORKIING;
                    cpu_work_stats = WORK_WAIT;
                    num_packet_wait = atoi(current_task[1].c_str());
                    std::string str_num_wait_packets = current_task[1];
                    // std::string str_src_mem_index = current_task[2];
                    // tell_mem_send_data(str_src_mem_index,  str_num_wait_packets,  id);
                }
                else if (current_task[0] == "cal"){
                    if(if_debug==1) std::cout << "current_task == cal" << std::endl;
                    cpu_status = WORKIING;
                    cpu_work_stats = WORK_CAL;

                    stringstream stream;            //声明一个stringstream变量
                    stream << current_task[1];      //向stream中插入字符串"1234"
                    stream >> cal_cycles;           // 初始化cal_cycles 
                    cycles_caled = 0;
                }
                else if (current_task[0] == "send"){
                    if(if_debug==1) std::cout << "current_task == send" << std::endl;
                    cpu_status = WORKIING;
                    cpu_work_stats = WORK_SEND;
                    packets_to_send = atoi(current_task[2].c_str());
                    send_dst = atoi(current_task[1].c_str());
                    packets_sent = 0;
                }
                else if (strstr(current_task[0].c_str(), "finish") != NULL ) { // 最后一行current_task[0]会多一个终结符
                    cpu_status = FINISH;
                    cpu_work_stats = WORK_IDLE;
                    std::cout << "cpu "<< id << " finished all " << "@ "<< curTick() << std::endl;
                }
            }
        }

        // working status
        else if (cpu_status == WORKIING){
            if (cpu_work_stats == WORK_WAIT){
                int packet_recv = recv_packets(id) - total_packet_recv_previous;
                if (packet_recv == num_packet_wait){
                    cpu_work_stats = WORK_IDLE;
                    cpu_status = IDLE;
                    total_packet_recv_previous += packet_recv;
                }
                // 否则维持wait状态
            }
            else if (cpu_work_stats == WORK_SEND){
                if (packets_sent == packets_to_send){  //TODO ++ packets_sent
                    cpu_work_stats = WORK_IDLE;
                    cpu_status = IDLE;
                }
                else {
                    sendAllowedThisCycle = true;
                }
            }

            else if (cpu_work_stats == WORK_CAL){
                if (cycles_caled == cal_cycles){
                    cpu_work_stats = WORK_IDLE;
                    cpu_status = IDLE;
                }
                else {
                    cycles_caled += 1;
                }
            }
        }
    } // end of DNN traffic

    if (++noResponseCycles >= responseLimit) {
        fatal("%s deadlocked at cycle %d\n", name(), curTick());
    }

    // make new request based on injection rate
    // (injection rate's range depends on precision)
    // - generate a random number between 0 and 10^precision
    // - send pkt if this number is < injRate*(10^precision)

    if (traffic != DNN_) {
        double injRange = pow((double) 10, (double) precision);
        unsigned trySending = random_mt.random<unsigned>(0, (int) injRange);
        if (trySending < injRate*injRange)
            sendAllowedThisCycle = true;
        else
            sendAllowedThisCycle = false;
    }

    // always generatePkt unless fixedPkts or singleSender is enabled
    if (sendAllowedThisCycle) {
        bool senderEnable = true;

        if (numPacketsMax >= 0 && numPacketsSent >= numPacketsMax)
            senderEnable = false;

        if (singleSender >= 0 && id != singleSender)
            senderEnable = false;
        if (senderEnable) {
            if (traffic == DNN_) {
                packets_sent += 1;
                generatePkt(send_dst);
            }
            else generatePkt(0);
        }
    }

    // Schedule wakeup
    if (curTick() >= simCycles)
        exitSimLoop("Network Tester completed simCycles");
    else {
        if (!tickEvent.scheduled())
            schedule(tickEvent, clockEdge(Cycles(1)));
    }
}

void
GarnetSyntheticTraffic::generatePkt(int send_dst)
{
    int num_destinations = numDestinations;
    int radix = (int) sqrt(num_destinations);
    unsigned destination = id;
    int dest_x = -1;
    int dest_y = -1;
    int source = id;
    int src_x = id%radix;
    int src_y = id/radix;

    if (singleDest >= 0)
    {
        destination = singleDest;
    } else if (traffic == UNIFORM_RANDOM_) {
        destination = random_mt.random<unsigned>(0, num_destinations - 1);
    } else if (traffic == BIT_COMPLEMENT_) {
        dest_x = radix - src_x - 1;
        dest_y = radix - src_y - 1;
        destination = dest_y*radix + dest_x;
    } else if (traffic == BIT_REVERSE_) {
        unsigned int straight = source;
        unsigned int reverse = source & 1; // LSB

        int num_bits = (int) log2(num_destinations);

        for (int i = 1; i < num_bits; i++)
        {
            reverse <<= 1;
            straight >>= 1;
            reverse |= (straight & 1); // LSB
        }
        destination = reverse;
    } else if (traffic == BIT_ROTATION_) {
        if (source%2 == 0)
            destination = source/2;
        else // (source%2 == 1)
            destination = ((source/2) + (num_destinations/2));
    } else if (traffic == NEIGHBOR_) {
            dest_x = (src_x + 1) % radix;
            dest_y = src_y;
            destination = dest_y*radix + dest_x;
    } else if (traffic == SHUFFLE_) {
        if (source < num_destinations/2)
            destination = source*2;
        else
            destination = (source*2 - num_destinations + 1);
    } else if (traffic == TRANSPOSE_) {
            dest_x = src_y;
            dest_y = src_x;
            destination = dest_y*radix + dest_x;
    } else if (traffic == TORNADO_) {
        dest_x = (src_x + (int) ceil(radix/2)) % radix;
        dest_y = src_y;
        destination = dest_y*radix + dest_x;
    } else if (traffic == DNN_) {
        destination = send_dst;
    }
    else {
        fatal("Unknown Traffic Type: %s!\n", traffic);
    }

    // The source of the packets is a cache.
    // The destination of the packets is a directory.
    // The destination bits are embedded in the address after byte-offset.
    Addr paddr =  destination;
    paddr <<= blockSizeBits;
    unsigned access_size = 1; // Does not affect Ruby simulation

    // Modeling different coherence msg types over different msg classes.
    //
    // GarnetSyntheticTraffic assumes the Garnet_standalone coherence protocol
    // which models three message classes/virtual networks.
    // These are: request, forward, response.
    // requests and forwards are "control" packets (typically 8 bytes),
    // while responses are "data" packets (typically 72 bytes).
    //
    // Life of a packet from the tester into the network:
    // (1) This function generatePkt() generates packets of one of the
    //     following 3 types (randomly) : ReadReq, INST_FETCH, WriteReq
    // (2) mem/ruby/system/RubyPort.cc converts these to RubyRequestType_LD,
    //     RubyRequestType_IFETCH, RubyRequestType_ST respectively
    // (3) mem/ruby/system/Sequencer.cc sends these to the cache controllers
    //     in the coherence protocol.
    // (4) Network_test-cache.sm tags RubyRequestType:LD,
    //     RubyRequestType:IFETCH and RubyRequestType:ST as
    //     Request, Forward, and Response events respectively;
    //     and injects them into virtual networks 0, 1 and 2 respectively.
    //     It immediately calls back the sequencer.
    // (5) The packet traverses the network (simple/garnet) and reaches its
    //     destination (Directory), and network stats are updated.
    // (6) Network_test-dir.sm simply drops the packet.
    //
    MemCmd::Command requestType;

    RequestPtr req = nullptr;
    Request::Flags flags;

    // Inject in specific Vnet
    // Vnet 0 and 1 are for control packets (1-flit)
    // Vnet 2 is for data packets (5-flit)
    int injReqType = injVnet;
    
    // 依据节点数目+src+dst确定注入的Vnet id
    // 如果是routerless设置，执行以下函数决定vnet；否则维持原始设置
    if (if_routerless == 1) {
        if (num_destinations==4) // 2*2 mesh
            injReqType = vnet_table_2_2[source][destination] + 2;
        else if (num_destinations==16) //4*4 mesh
            injReqType = vnet_table_4_4[source][destination] + 2;
        else if (num_destinations==36) //6*6 mesh
            injReqType = vnet_table_6_6[source][destination] + 2;
        else if (num_destinations==64) //8*8 mesh
            injReqType = vnet_table_8_8[source][destination] + 2;
        else {
            std::cout << "routerless mesh size is not supported now" << std::endl;
            assert(0);} 
    }

    if (injReqType==-1) // -1 代表random(0,2),其他代表自己本身
    {
        // randomly inject in any vnet
        injReqType = random_mt.random(0, 2);
    }
    if (injReqType==-2) // -2 代表random(2,所有vnets)
    {
        injReqType = random_mt.random(2, 9); //fanxitodo:传输参数进来
    }

    if (injReqType == 0) {
        // generate packet for virtual network 0
        requestType = MemCmd::ReadReq;
        req = new Request(paddr, access_size, flags, masterId);
    } else if (injReqType == 1) {
        // generate packet for virtual network 1
        requestType = MemCmd::ReadReq;
        flags.set(Request::INST_FETCH);
        req = new Request(
            0, 0x0, access_size, flags, masterId, 0x0, 0);
        req->setPaddr(paddr);
    } 
    else if (injReqType == 2) {
        // generate packet for virtual network 2
        requestType = MemCmd::WriteReq;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else  if (injReqType == 3) {
        requestType = MemCmd::WriteReq1;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else  if (injReqType == 4) {
        requestType = MemCmd::WriteReq2;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 5) {
        requestType = MemCmd::WriteReq3;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 6) {
        requestType = MemCmd::WriteReq4;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 7) {
        requestType = MemCmd::WriteReq5;
        req = new Request(paddr, access_size, flags, masterId);
    }

    else if (injReqType == 8) {
        requestType = MemCmd::WriteReq6;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 9) {
        requestType = MemCmd::WriteReq7;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 10) {
        requestType = MemCmd::WriteReq8;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 11) {
        requestType = MemCmd::WriteReq9;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 12) {
        requestType = MemCmd::WriteReq10;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 13) {
        requestType = MemCmd::WriteReq11;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 14) {
        requestType = MemCmd::WriteReq12;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 15) {
        requestType = MemCmd::WriteReq13;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 16) {
        requestType = MemCmd::WriteReq14;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 17) {
        requestType = MemCmd::WriteReq15;
        req = new Request(paddr, access_size, flags, masterId);
    }
    else if (injReqType == 18) {
        requestType = MemCmd::WriteReq16;
        req = new Request(paddr, access_size, flags, masterId);
    }

    req->setContext(id);

    //No need to do functional simulation
    //We just do timing simulation of the network

    DPRINTF(GarnetSyntheticTraffic,
            "Generated packet with destination %d, embedded in address %x\n",
            destination, req->getPaddr());

    PacketPtr pkt = new Packet(req, requestType);
    pkt->dataDynamic(new uint8_t[req->getSize()]);
    pkt->senderState = NULL;

    sendPkt(pkt);
}

void
GarnetSyntheticTraffic::initTrafficType()
{
    trafficStringToEnum["bit_complement"] = BIT_COMPLEMENT_;
    trafficStringToEnum["bit_reverse"] = BIT_REVERSE_;
    trafficStringToEnum["bit_rotation"] = BIT_ROTATION_;
    trafficStringToEnum["neighbor"] = NEIGHBOR_;
    trafficStringToEnum["shuffle"] = SHUFFLE_;
    trafficStringToEnum["tornado"] = TORNADO_;
    trafficStringToEnum["transpose"] = TRANSPOSE_;
    trafficStringToEnum["uniform_random"] = UNIFORM_RANDOM_;
    trafficStringToEnum["DNN"] = DNN_;
}

void
GarnetSyntheticTraffic::doRetry()
{
    if (cachePort.sendTimingReq(retryPkt)) {
        retryPkt = NULL;
    }
}

void
GarnetSyntheticTraffic::printAddr(Addr a)
{
    cachePort.printAddr(a);
}


GarnetSyntheticTraffic *
GarnetSyntheticTrafficParams::create()
{
    return new GarnetSyntheticTraffic(this);
}
