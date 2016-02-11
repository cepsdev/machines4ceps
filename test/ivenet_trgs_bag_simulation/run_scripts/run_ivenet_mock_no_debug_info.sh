#!/bin/bash
#Starts an ivenet mock in quiet mode
../../x86/sm --quiet \
 common/common.ceps \
 communication/comm.ceps \
 communication/writer_ivenet_msgs.ceps \
 communication/reader_trgs_bag_mast_msgs.ceps models/ivenet_mock.ceps