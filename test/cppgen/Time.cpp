#include <cstdlib>
#include <string>
#include <iostream>
#include <time.h>
#include "common.h"
#include "Time.h"

//---------------------------------------------------------------------------
// date -s '2016-2-1 11:15:00'
//---------------------------------------------------------------------------
int Set_Time(int year, int month, int day, int hour, int minute, int second)
{  
    tm tm_time;
    tm_time.tm_year = year    - 1900;
    tm_time.tm_mon  = month   -    1;
    tm_time.tm_mday = day;
    tm_time.tm_hour = hour;
    tm_time.tm_min  = minute;
    tm_time.tm_sec  = second;
    
    time_t rawtime;
    rawtime = mktime (&tm_time);
    
    int result = stime(&rawtime);
    
    if(result != 0)
        std::cout << "Set_Time() fails to set local system time" << std::endl;
    return result;
}

int Get_Time(std::string unit) {
   // current date/time based on current system
   time_t now = time(0);
   tm *ltm = localtime(&now);
   
   if(unit == "year") return ltm->tm_year + 1900;
   else
   if(unit == "month") return ltm->tm_mon + 1;
   else
   if(unit == "day") return ltm->tm_mday;
   else
   if(unit == "hour") return ltm->tm_hour;
   else
   if(unit == "minute") return ltm->tm_min;
   else
   if(unit == "second") return ltm->tm_sec;
   else
       return -1;
}