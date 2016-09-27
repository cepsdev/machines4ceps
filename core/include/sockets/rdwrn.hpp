//Taken from Kerrisk: The Linux Programming Interface

#ifndef SOCKETS_RDWRN_HPP_INC
#define SOCKETS_RDWRN_HPP_INC

#include <sys/types.h>

ssize_t readn(int fd, void * buffer, size_t count);
ssize_t writen(int fd, void * buffer, size_t count);


#endif
