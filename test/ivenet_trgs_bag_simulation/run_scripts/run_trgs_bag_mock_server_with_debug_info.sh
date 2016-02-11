#!/bin/bash
#Starts the trgs_bag_mock_server in quiet model
#Enables remote monitoring, flag: --monitorPORT
../../eclipse/Debug/statemachines --debug --monitor3000\
 common/common.ceps\
 communication/comm.ceps\
 communication/reader_ivenet_msgs.ceps\
 communication/writer_trgs_bag_mast_msgs.ceps\
 models/trgs_bag_mock_server.ceps