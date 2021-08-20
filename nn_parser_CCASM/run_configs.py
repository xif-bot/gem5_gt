

method ="GA_RF"  # "comp_group_seq"  "comp_group_rdm" "GA_RF"   "neuron_group_seq"  "neuron_group_rdm" 
task = "alexnet"          # "lenet"    "VGG16"            "resnet_50"  "alexnet"
# 2D Mesh规模 noc_node_num = NoC_w *NoC_w
NoC_w = 6
PE_ability_list = [700,800,900,1000,1100,1200,1300,1400,1500]
# PE_ability_list = [50,100,150,200,250,300,350,400,450,500,550,600]
# PE_ability_list = [310,320,330,340,350,360,370,380,390,400,410,420,430,440,450,460,470,480,490,500,510,520,530,540,550,560,570,580,590,600]
# PE_ability_list = [10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,210,220,230,240]


#硬件参数
group_size = 100

neu_per_flit = 2
flit_per_pkt = 5
NOC_NODE_NUM = NoC_w*NoC_w

# 计算、发送任务的缩放比例

# 算法一的节点范围
max_node_num = NOC_NODE_NUM
min_node_num = 4

debug = 0
congestion_ratio = 0