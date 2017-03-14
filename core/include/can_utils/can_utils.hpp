#ifndef INC_SM4CEPS_CAN_UTILS_HPP
#define INC_SM4CEPS_CAN_UTILS_HPP

#ifdef __gnu_linux__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <ctype.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>


#include <string>

namespace sm4ceps{ namespace can_utils{

struct can_socket_err{
    std::string msg;
    int errid;
};

int create_and_bind_raw_can_socket(std::string can_id);
void send_can_frame(int s, std::uint8_t* data, size_t len, canid_t id = 0);

} }

#endif

#endif