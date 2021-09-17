# Hello World demo

A simple documentation to get the hello world demo running on Ubuntu 20.04

* Install QT 
* Open the .pro file on QT. Compile!
* It creates a new folder with a name similar to "build-hello_world-Desktop_Qt_5_15_2_GCC_64bit-Debug" on the path machines4ceps/demo
* Open a new terminal go to the path containing spec cd ../spec/can_comm
* sudo ./setup_vcan0 
* candump vcan0
* On a new terminal go to the directory build-hello_world-Desktop_Qt_5_15_2_GCC_64bit-Debug and run ./hello_world
* The candump displays a can message 
