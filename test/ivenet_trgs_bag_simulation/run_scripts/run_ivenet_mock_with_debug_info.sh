#!/bin/bash
#Starts ivenet mock with debug messages turned on
../../x86/sm --debug \
 common/common.ceps \
 communication/comm.ceps \
 communication/writer_ivenet_msgs.ceps \
 communication/reader_trgs_bag_mast_msgs.ceps models/ivenet_mock.ceps
