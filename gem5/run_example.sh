cp ~/noc/NN-Parser/task/task_lenet_2/* ~/noc/routerless/gem5_gt/dnn_task/lenet_rf_ga/
cd ~/noc/routerless/gem5_gt/gem5/
./build/Garnet_standalone/gem5.opt configs/example/garnet_synth_traffic.py \
--network=garnet2.0 \
--num-cpus=144 \
--num-dirs=144 \
--topology=Mesh_XY \
--mesh-rows=12 \
--sim-cycles=4000 \
--inj-vnet=2 \
--injectionrate=0.02 \
--synthetic=DNN \
--dnn_task=lenet_rf_ga \
--routing-algorithm=xy \
--if_routerless=0 \
--link_width_bits=128 \
--if_debug=0
