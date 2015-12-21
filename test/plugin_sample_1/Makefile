#
# sm4ceps - Makefile
#

includes := -I"." -I"../.."
cflags := -g3 -Wall -MD -fmessage-length=0 -std=c++1y 

all: antriebe.so

antriebe.so: Antriebe.o
	$(CXX)   $(cflags) -shared  Antriebe.o -o Antriebe.so
Antriebe.o: Antriebe.cpp
	$(CXX)   $(cflags) -fPIC  $(includes) -c Antriebe.cpp -o Antriebe.o

clean:

	rm Antriebe.o
	rm Antriebe.so 

