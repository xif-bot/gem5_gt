./build/Garnet_standalone/gem5.opt configs/example/garnet_synth_traffic.py \
--network=garnet2.0 \
--num-cpus=64 \
--num-dirs=64 \
--topology=Mesh_XY \
--mesh-rows=8 \
--sim-cycles=10000 \
--inj-vnet=4 \
--injectionrate=0.1 \
--synthetic=uniform_random \
--routing-algorithm=xy \
--num_vnets=19