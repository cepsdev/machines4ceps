#include <iostream>
#include "core/include/can_utils/can_utils.hpp"
#include "out.hpp"

extern void user_defined_init();

int main(int argc, char** argv){
  using namespace sm4ceps::can_utils;
  user_defined_init();

  try{
   auto s = create_and_bind_raw_can_socket("vcan0");

   std::uint8_t test[] = {1,2,3,4};
   send_can_frame(s,test,sizeof(test),1);

  } catch (can_socket_err err){
   std::cerr << err.msg <<": "<< strerror(err.errid) << std::endl;
   return 1;
  }
  return 0;
}




