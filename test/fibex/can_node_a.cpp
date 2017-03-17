#include <iostream>
#include "core/include/can_utils/can_utils.hpp"
#include "out.hpp"

int main(int argc, char** argv){
  using namespace sm4ceps::can_utils;
  user_defined_init();
  try{
   auto s = create_and_bind_raw_can_socket("vcan0");

   for (auto i = 0; i!=10;++i){
	   systemstates::rpm = (double)i;
       auto frame = create_frame_Engine_RPM();
	   send_can_frame(s,(std::uint8_t*)&frame,sizeof(frame));
   }
  } catch (can_socket_err err){
   std::cerr << err.msg <<": "<< strerror(err.errid) << std::endl;
   return 1;
  }
  return 0;
}




