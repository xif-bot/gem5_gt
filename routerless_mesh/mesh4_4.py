import numpy as np
import sys
from random import choice

# 12 13 14 15
# 08 09 10 11
# 04 05 06 07
# 00 01 02 03

ring = []
ring.append([0,1,2,3,7,11,15,14,13,12,8,4]) # ring0
ring.append([5,9,10,6]) # ring1
ring.append([9,5,6,10]) # ring2
ring.append([12,13,9,5,1,0,4,8]) # ring3
ring.append([13,14,15,11,7,3,2,1,5,9]) # ring4
ring.append([12,13,14,10,6,2,1,0,4,8]) # ring5
ring.append([14,15,11,7,3,2,6,10]) # ring6
ring.append([12,13,14,15,11,10,9,8]) # ring7
ring.append([8,9,10,11,7,6,5,4]) # ring8
ring.append([4,5,6,7,3,2,1,0]) # ring9

vnet_num = 7
# the ring= [0,1,2,3,4,5,6,7,8,9]
ring2vnet = [0,0,4,1,6,2,1,3,5,3]

link = []
for ring_id in range (len(ring)):
    link.append([])
    for node in range (len(ring[ring_id])):
        link[ring_id].append(0)


min_hops = np.ones((16,16))
ring_record = np.zeros((16,16))
vnet_record = np.zeros((16,16))
route_info = np.zeros((len(ring),16))

for src in range (16):
    for dst in range (16):
        all_hops = []
        possible_id = []
        min_hops[src][dst] = 999
        for ring_id in range (10):
            src_exist = (src in ring[ring_id])
            dst_exist = (dst in ring[ring_id])
            # calculate the hop count of in the ring[ring_id]
            if (src_exist and dst_exist):
                src_loc = ring[ring_id].index(src)
                dst_loc = ring[ring_id].index(dst)
                if src_loc <= dst_loc:
                    hop_count = dst_loc - src_loc
                else:
                    hop_count = dst_loc + len(ring[ring_id]) - src_loc
                # print ("src=",src,"dst=",dst,"ring_id=",ring_id,"src_loc",src_loc,"dst_loc",dst_loc,"hop_count",hop_count)
            else: hop_count = 999
            # record all ! vnet & hop count
            all_hops.append(hop_count)
        the_min_hop = min(all_hops)
        min_hops[src][dst] = the_min_hop
        for id in range(len(all_hops)):
            if (all_hops[id]==the_min_hop): possible_id.append(id)
        print ("from",src,"to",dst,all_hops, possible_id)
        final_ringid = choice(possible_id)
        ring_record[src][dst] = final_ringid
        vnet_record[src][dst] = ring2vnet[final_ringid]



print ("min_hops:")
print (min_hops)

print ("ring_record",ring_record)
print ("mean min_hops",np.sum(min_hops)/(16*15)) 

# cal the route info
for ring_id in range (len(ring)):
    vnet_id = ring2vnet[ring_id]
    for src_id in range (len(ring[ring_id])):
        dst_id = (src_id+1)%(len(ring[ring_id]))
        src = ring[ring_id][src_id]
        dst = ring[ring_id][dst_id]
        src_x = src % 4
        src_y = int(src / 4)
        dst_x = dst % 4
        dst_y = int(dst / 4)
        x_equal = (dst_x == src_x)
        y_equal = (dst_y == src_y)
        # print (src, src_x, src_y, dst, dst_x,dst_y)
        if (x_equal == 1):
            if (dst_y > src_y) :  route_info[vnet_id][src] = 1; # north
            elif (dst_y < src_y): route_info[vnet_id][src] = 2; # south
            else: print ("fatal: cannot routing!")
        elif (y_equal == 1):
            if (dst_x > src_x) :  route_info[vnet_id][src] = 4; # east
            elif (dst_x < src_x): route_info[vnet_id][src] = 3; # west
            else: print ("fatal: cannot routing!")
        else: 
            print ("fatal: cannot routing!")
    # print(route_info[vnet_id])

# record all infos
with open("ring_4_4.txt","w") as f:
    print("{",file=f)
    print ("ring_record",file=f)
    for src in range (16):
        print(" {",file=f,end="\t")
        for dst in range (16):
            if dst == 15:
                print (int(ring_record[src][dst]),file=f,end=" \t")
            else:
                print (int(ring_record[src][dst]),file=f,end=",\t")
        if src == 15:
            print(" }",file=f)
        else:
            print(" },",file=f)
    print("}",file=f)

    print ("vnet_record",file=f)
    print("{",file=f)
    for src in range (16):
        print(" {",file=f,end="\t")
        for dst in range (16):
            if dst == 15:
                print (int(vnet_record[src][dst]),file=f,end=" \t")
            else:
                print (int(vnet_record[src][dst]),file=f,end=",\t")
        if src == 15:
            print(" }",file=f)
        else:
            print(" },",file=f)
    print("}",file=f)

    print ("route_info",file=f)
    print("{",file=f)
    for vnet_id in range (vnet_num):
        print(" {",file=f,end="\t")
        for src in range (16):
            if src == 15:
                print (int(route_info[vnet_id][src]),file=f,end=" \t")
            else:
                print (int(route_info[vnet_id][src]),file=f,end=",\t")
        if vnet_id == vnet_num:
            print(" }",file=f)
        else:
            print(" },",file=f)
    print("}",file=f)


        






