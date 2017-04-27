#include "core/include/sockets/rdwrn.hpp"
#include <unistd.h>
#include <errno.h>

//Taken from Kerrisk: The Linux Programming Interface
ssize_t readn(int fd, void * buffer, size_t n){
 auto buf = (char*) buffer ;
 size_t totRead = 0;
 for( ; totRead < n;){
  auto numRead = read(fd,buf,n - totRead);
  if (numRead == 0)
   return totRead;
  if (numRead == -1){
   if(errno == EINTR)
    continue;
   else
    return -1;
  }
  totRead += numRead;
  buf += numRead;
 }
 return totRead;
}

ssize_t writen(int fd, void * buffer, size_t n){
 const char* buf = (char*) buffer;
 size_t totWritten = 0;
 for(; totWritten < n;){
  auto numWritten = write(fd,buf,n - totWritten);
  if (numWritten <= 0) {
   if(numWritten == -1 && errno == EINTR)
    continue;
   else
    return -1;
  }
 }
 return totWritten;
}

