#include "core/include/can_utils/can_utils.hpp"

#ifdef __gnu_linux__

int sm4ceps::can_utils::create_and_bind_raw_can_socket(std::string can_id){
 int s;
 struct sockaddr_can addr;
 struct ifreq ifr;
 if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
  throw sm4ceps::can_utils::can_socket_err{"socket call failed",errno};
 strcpy(ifr.ifr_name, can_id.c_str());
 if (-1 == ioctl(s, SIOCGIFINDEX, &ifr))
  throw sm4ceps::can_utils::can_socket_err{"socket call failed",errno};
 addr.can_family  = AF_CAN;
 addr.can_ifindex = ifr.ifr_ifindex;
 if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  throw sm4ceps::can_utils::can_socket_err{"bind call failed",s};
 return s;
}

void sm4ceps::can_utils::send_can_frame(int s, std::uint8_t* data, size_t len, canid_t id){
 struct can_frame frame;
 memset(&frame,0,sizeof(frame));
 memcpy(frame.data,(char*)data,std::min((int)len,CAN_MAX_DLEN));
 frame.can_id = id;
 frame.can_dlc = std::min((int)len,CAN_MAX_DLEN);
 auto nbytes = write(s, &frame, sizeof(frame));
 if (nbytes != sizeof(frame))
	 throw sm4ceps::can_utils::can_socket_err{"write failed",errno};
}

#endif