# http://stackoverflow.com/questions/21022749/how-to-create-virtual-can-port-on-linux-c
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0