
#!/bin/bash

mkdir bin 2>/dev/null

cd bin 
rm CMakeFiles -rf 2>/dev/null  
rm cmake_install.cmake -f 2>/dev/null
rm CMakeCache.txt -f 2>/dev/null
rm Makefile -f 2>/dev/null
rm lib* -f 2>/dev/null

CEPSCORE=../../../ceps/core MACHINES4CEPS=../../../machines4ceps LOG4CEPS=../../../log4ceps cmake .. && make -B
cd ..					
				