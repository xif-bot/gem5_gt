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

#ifndef __CPU_GARNET_SYNTHETIC_TRAFFIC_HH__
#define __CPU_GARNET_SYNTHETIC_TRAFFIC_HH__

#include <set>

#include "base/statistics.hh"
#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/GarnetSyntheticTraffic.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/sim_object.hh"
#include "sim/stats.hh"

enum TrafficType {BIT_COMPLEMENT_ = 0,
                  BIT_REVERSE_ = 1,
                  BIT_ROTATION_ = 2,
                  NEIGHBOR_ = 3,
                  SHUFFLE_ = 4,
                  TORNADO_ = 5,
                  TRANSPOSE_ = 6,
                  UNIFORM_RANDOM_ = 7,
                  DNN_ = 8,
                  NUM_TRAFFIC_PATTERNS_};

class Packet;
class GarnetSyntheticTraffic : public MemObject
{
  public:
    int cal_cycles;
    int packets_to_send ;
    int send_dst;
    int send_cmd_dst;
    int packets_sent;
    int cpu_status;
    int num_packet_wait;
    int downstream_id;
    int cycles_caled;
    int total_packet_recv_previous;
    int cpu_work_stats;
    std::string current_task_line;
    int current_line_num;
    int Repeat_Start_line;
    int cur_pic;
    int num_recv_cmd_packet;
    int get_task(int id,int line_num);
    int check_downstream(int id);
    void update_cur_pic(int id, int cur_pic_id);
    void tell_mem_send_data(std::string src_mem_index,std::string num_wait_packets,int id);
    int time_cal  ;
    int time_wait  ;
    int time_wait_cmd  ;
    int time_send ;

    typedef GarnetSyntheticTrafficParams Params;
    GarnetSyntheticTraffic(const Params *p);

    virtual void init();

    // main simulation loop (one cycle)
    void tick();

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);

    /**
     * Print state of address in memory system via PrintReq (for
     * debugging).
     */
    void printAddr(Addr a);

  protected:
    EventFunctionWrapper tickEvent;

    class CpuPort : public MasterPort
    {
        GarnetSyntheticTraffic *tester;

      public:
        int num_packet_recv;

        CpuPort(const std::string &_name, GarnetSyntheticTraffic *_tester)
            : MasterPort(_name, _tester), tester(_tester)
        { }

      protected:

        virtual bool recvTimingResp(PacketPtr pkt);

        virtual void recvReqRetry();
    };

    CpuPort cachePort;

    class GarnetSyntheticTrafficSenderState : public Packet::SenderState
    {
      public:
        /** Constructor. */
        GarnetSyntheticTrafficSenderState(uint8_t *_data)
            : data(_data)
        { }

        // Hold onto data pointer
        uint8_t *data;
    };

    PacketPtr retryPkt;
    unsigned size;
    int id;

    std::map<std::string, TrafficType> trafficStringToEnum;

    unsigned blockSizeBits;

    Tick noResponseCycles;

    int numDestinations;
    Tick simCycles;
    int numPacketsMax;
    int numPacketsSent;
    int singleSender;
    int singleDest;

    std::string trafficType; // string
    TrafficType traffic; // enum from string
    double injRate;
    int injVnet;
    int if_routerless;
    int if_debug;
    std::string dnn_task;
    int precision;

    const Cycles responseLimit;

    MasterID masterId;

    void completeRequest(PacketPtr pkt);
    int recv_packets(int id);

    void generatePkt(int send_dst);
    void sendPkt(PacketPtr pkt);
    void initTrafficType();

    void doRetry();

    friend class MemCompleteEvent;
    int vnet_table_2_2[4][4] = { {0, 0, 1, 1}, \
                                 {1, 0, 1, 0}, \
                                 {0, 0, 1, 1}, \
                                 {1, 1, 0, 0} };

    int vnet_table_4_4[16][16] = { {	2,	0,	0,	0,	3,	3,	3,	3,	2,	1,	2,	0,	2,	2,	2,	0 }, \
                                   {	3,	6,	0,	0,	1,	6,	3,	0,	2,	6,	2,	0,	2,	6,	6,	6 }, \
                                   {	2,	6,	1,	0,	3,	6,	1,	0,	2,	6,	1,	0,	2,	6,	1,	0 }, \
                                   {	3,	6,	3,	3,	3,	6,	1,	0,	0,	6,	1,	0,	0,	6,	0,	0 }, \
                                   {	0,	0,	0,	0,	1,	3,	3,	3,	5,	5,	5,	5,	2,	1,	2,	0 }, \
                                   {	1,	1,	3,	3,	5,	5,	3,	3,	5,	0,	0,	6,	1,	6,	6,	6 }, \
                                   {	2,	2,	2,	3,	5,	0,	4,	3,	5,	0,	4,	1,	2,	2,	1,	1 }, \
                                   {	3,	6,	1,	3,	5,	5,	5,	6,	5,	6,	1,	0,	0,	0,	0,	0 }, \
                                   {	0,	0,	0,	0,	0,	1,	2,	5,	3,	5,	5,	5,	3,	1,	2,	3 }, \
                                   {	1,	1,	6,	6,	1,	4,	4,	5,	3,	4,	0,	5,	3,	6,	6,	6 }, \
                                   {	2,	2,	2,	1,	5,	4,	2,	5,	3,	3,	5,	5,	3,	3,	1,	1 }, \
                                   {	0,	6,	6,	6,	5,	5,	5,	1,	3,	3,	3,	0,	0,	0,	0,	0 }, \
                                   {	0,	1,	2,	0,	0,	1,	2,	0,	0,	1,	2,	3,	3,	1,	2,	3 }, \
                                   {	1,	1,	2,	6,	0,	1,	2,	6,	0,	1,	2,	6,	0,	6,	3,	6 }, \
                                   {	0,	2,	2,	1,	0,	6,	2,	6,	0,	3,	2,	6,	0,	0,	6,	6 }, \
                                   {	0,	6,	1,	6,	0,	6,	1,	1,	0,	3,	3,	1,	0,	0,	0,	1 }};

  int vnet_table_6_6[36][36];
  int vnet_table_8_8[64][64];


};

#endif // __CPU_GARNET_SYNTHETIC_TRAFFIC_HH__
