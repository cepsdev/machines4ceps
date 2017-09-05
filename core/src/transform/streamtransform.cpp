#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <memory>
#include <iostream>
#include <sys/stat.h>
#include <locale>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <climits>
#include <map>
#include "core/include/transform/streamtransform.hpp"


bool operator == (nonmutable_string<char> const &  lhs ,  nonmutable_string<char> const &  rhs)
{
 if (lhs.length() != rhs.length()) return false;
 if (lhs.length() == 0) return true;
 return 0 == strncmp(rhs.data(),lhs.data(),lhs.length());
}

bool operator != (nonmutable_string<char> const &  lhs ,  nonmutable_string<char> const &  rhs)
{

 return !(lhs == rhs);
}

bool operator < (nonmutable_string<char> const &  lhs ,  nonmutable_string<char> const &  rhs)
{

 auto r = strncmp(rhs.data(),lhs.data(),std::min(lhs.length(),rhs.length()) );
 if (r == 0)
  return (lhs.length() < rhs.length() );
 return r < 0 ? true: false;
}

bool operator == (nonmutable_string<char> const &  lhs ,  const char * rhs)
{
 if (lhs.length() != strlen(rhs) ) return false;
 return 0 == strncmp(rhs,lhs.data(),lhs.length());
}

bool operator != (nonmutable_string<char> const &  lhs , const char * rhs)
{
 return !(lhs == rhs);
}

std::ostream& operator << (std::ostream & o, nonmutable_string<char> const & s)
{
  if (s.length() == 0) return o;
  for(std::size_t i = 0; i < s.length();++i) o << s[i];
  return o;
}


bool readfile_to_memory(Memory<char>& mem, const char* filename )
{
  struct stat statbuf;

  if (stat(filename,&statbuf) == -1) return false;

  auto inputfd = open(filename,O_RDONLY);
  if (inputfd == -1) return false;

  mem.ptr_ = new char[statbuf.st_size];
  if (mem.ptr_ == nullptr) return false;
  mem.size_ = statbuf.st_size;
  mem.pos_ = 0;
  intmax_t readbytes_total = 0;
  for(intmax_t readbytes = 0;readbytes = read(inputfd,mem.ptr_ + readbytes_total, mem.size_-readbytes_total);readbytes_total+=readbytes);


  return true;
}


const char *   default_ops[] = {  "{", "}",";","\n"," ","=>",
                            "<",">","$",":","/","|","(",
                             ")","=","==","!=","&&","||",
                             "!",",","+","-","*",".","@","\\","[","]","'", nullptr};
