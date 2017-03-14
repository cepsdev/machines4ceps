#
# can_node example
#

includes :=  -I"." -I"../../"
cflags := -O0 -g3 -Wall -fmessage-length=0 -std=c++11
libs :=

all: can_node_a 
can_node_a: out.o out_frames.o can_node_a.o can_utils.o
	g++   $(cflags) $(includes) can_node_a.o out.o out_frames.o can_utils.o -o can_node_a 

out.o: out.cpp out.hpp out_frames.hpp
	g++ -c $(cflags) out.cpp $(includes) -o out.o
out_frames.o: out_frames.cpp out_frames.hpp out.hpp
	g++ -c $(cflags) out_frames.cpp $(includes) -o out_frames.o
can_node_a.o: out_frames.hpp out.hpp can_node_a.cpp
	g++ -c $(cflags) can_node_a.cpp $(includes) -o can_node_a.o
can_utils.o: ../../core/include/can_utils/can_utils.hpp ../../core/src/can_utils/can_utils.cpp
	g++ -c $(cflags) ../../core/src/can_utils/can_utils.cpp $(includes) -o can_utils.o

clean:
	rm -f *.o
	rm -f can_node

