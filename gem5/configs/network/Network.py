# Copyright (c) 2016 Georgia Institute of Technology
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Tushar Krishna

import math
import m5
from m5.objects import *
from m5.defines import buildEnv
from m5.util import addToPath, fatal

def define_options(parser):
    # By default, ruby uses the simple timing cpu
    parser.set_defaults(cpu_type="TimingSimpleCPU")

    parser.add_option("--topology", type="string", default="Crossbar",
                      help="check configs/topologies for complete set")
    parser.add_option("--routing-table", type="string", default="",
                      help="add routing table")
    parser.add_option("--mesh-rows", type="int", default=0,
                      help="the number of rows in the mesh topology")
    parser.add_option("--circ-obr", type="string", default="",
                      help="the number of circ in the circ topology")
    parser.add_option("--network", type="choice", default="simple",
                      choices=['simple', 'garnet2.0'],
                      help="'simple'|'garnet2.0'")
    parser.add_option("--router-latency", action="store", type="int",
                      default=1,
                      help="""number of pipeline stages in the garnet router.
                            Has to be >= 1.
                            Can be over-ridden on a per router basis
                            in the topology file.""")
    parser.add_option("--link-latency", action="store", type="int", default=1,
                      help="""latency of each link the simple/garnet networks.
                            Has to be >= 1.
                            Can be over-ridden on a per link basis
                            in the topology file.""")
    parser.add_option("--link-width-bits", action="store", type="int",
                      default=128,
                      help="width in bits for all links inside garnet.")
    parser.add_option("--vcs-per-vnet", action="store", type="int", default=4,
                      help="""number of virtual channels per virtual network
                            inside garnet network.""")
    parser.add_option("--buffers-per-ctrl-vc", action="store", type="int", default=16,
                      help="number of buffers in each ctrl VC.")
    parser.add_option("--buffers-per-data-vc", action="store", type="int", default=4,
                      help="number of buffers in each data VC.")
    parser.add_option("--wormhole", action="store_true",
                      default=False,
                      help="enable wormhole")
    parser.add_option("--routing-algorithm", type="choice",
                      default="xy",
                      choices=['table', 'xy', 'turn_model_oblivious',
                                'turn_model_adaptive', 'random_oblivious',
                                'random_adaptive', 'custom'],
                      help="""routing algorithm in network.
                            'table' | 'xy' | 'turn_model_oblivious' |
                            'turn_model_adaptive' | 'random_oblivious' |
                            'random_adaptive' | 'custom'.
                            Implementation: see garnet2.0/RoutingUnit.cc""")
    parser.add_option("--network-fault-model", action="store_true",
                      default=False,
                      help="""enable network fault model:
                            see src/mem/ruby/network/fault_model/""")
    parser.add_option("--garnet-deadlock-threshold", action="store",
                      type="int", default=5000,
                      help="network-level deadlock threshold.")


def create_network(options, ruby):
    print ("fanxi added 0511, in network.py, create_network(options, ruby)")
    # Set the network classes based on the command line options
    if options.network == "garnet2.0":
        NetworkClass = GarnetNetwork
        IntLinkClass = GarnetIntLink
        ExtLinkClass = GarnetExtLink
        RouterClass = GarnetRouter
        InterfaceClass = GarnetNetworkInterface

    else:
        NetworkClass = SimpleNetwork
        IntLinkClass = SimpleIntLink
        ExtLinkClass = SimpleExtLink
        RouterClass = Switch
        InterfaceClass = None

    # Instantiate the network object
    # so that the controllers can connect to it.
    network = NetworkClass(ruby_system = ruby, topology = options.topology,
            routers = [], ext_links = [], int_links = [], netifs = [])

    return (network, IntLinkClass, ExtLinkClass, RouterClass, InterfaceClass)

def init_network(options, network, InterfaceClass):

    print ("fanxi added 0511, in Network.py, in func init_network()")
    if options.network == "garnet2.0":
        network.num_rows = options.mesh_rows
        network.ni_flit_size = options.link_width_bits / 8
        network.garnet_deadlock_threshold = options.garnet_deadlock_threshold

    if options.routing_algorithm == "table":
        network.routing_algorithm = 0
    elif options.routing_algorithm == "xy":
        network.routing_algorithm = 1
    elif options.routing_algorithm == "turn_model_oblivious":
        network.routing_algorithm = 2
    elif options.routing_algorithm == "turn_model_adaptive":
        network.routing_algorithm = 3
    elif options.routing_algorithm == "random_oblivious":
        network.routing_algorithm = 4
    elif options.routing_algorithm == "random_adaptive":
        network.routing_algorithm = 5
    elif options.routing_algorithm == "custom":
        network.routing_algorithm = 6
    else:
        network.routing_algorithm = 0
    network.routing_table = options.routing_table
    network.circ_obr = options.circ_obr

    if options.wormhole:
        network.vcs_per_vnet = 1
        network.buffers_per_ctrl_vc = options.vcs_per_vnet
        network.buffers_per_data_vc = options.buffers_per_data_vc
    else:
        network.vcs_per_vnet = options.vcs_per_vnet
        network.buffers_per_ctrl_vc = options.buffers_per_ctrl_vc
        network.buffers_per_data_vc = options.buffers_per_data_vc

    if options.network == "simple":
        network.setup_buffers()

    if InterfaceClass != None:
        netifs = [InterfaceClass(id=i) \
                  for (i,n) in enumerate(network.ext_links)]
        network.netifs = netifs

    if options.network_fault_model:
        assert(options.network == "garnet2.0")
        network.enable_fault_model = True
        network.fault_model = FaultModel()
