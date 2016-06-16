# http://stackoverflow.com/questions/21022749/how-to-create-virtual-can-port-on-linux-c
sudo modprobe vcan
sudo ip link add dev vcan1 type vcan
sudo ip link set up vcan1