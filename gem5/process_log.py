import sys


import matplotlib.pyplot as plt

assert (len(sys.argv)) == 3
logfile = str(sys.argv[1])
final_node = str(sys.argv[2])
match_line = "cpu " +final_node+" finished all"
finish_time = []
one_pic_time = []

with open (logfile,'r') as file:
    for line in file.readlines():
        if match_line in line:
            line_content = line.split()
            finish_time.append(int(line_content[5]))

for id in range (len(finish_time)):
    if id==0:
        one_pic_time.append(finish_time[id])
    else:
        one_pic_time.append(finish_time[id] - finish_time[id-1])

print (finish_time)
plt.plot(finish_time)
plt.ylabel('total time')
plt.show()
print (one_pic_time)
plt.plot(one_pic_time)
plt.ylabel('one pic processing time')
plt.show()
print ("pic_processed = ",len(finish_time))
throughput = (finish_time[-1]-finish_time[0])/ (len(finish_time)-1)
latency =  finish_time[0] #只是第一张延迟，不是平均每张的
print ("throughput=",throughput)
print ("latency=",latency)
