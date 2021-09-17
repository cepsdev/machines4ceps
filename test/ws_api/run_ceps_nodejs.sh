#!/bin/bash

# Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).
#
#Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.


# Starts ceps which runs an empty simulation in server mode. 
# ceps is listening on port 8182 for inbound WebSocket connections.
# The client, a node.js program, connects with ceps and fires events which trigger transitions in the state machines controlled by the ceps server.
# Furthermore the client monitors variables updated by the simulation executed by the cep server.


echo                 "################################################################"
echo -e "\e[38;5;115mWS API: NodeJs <-> ceps. Test will stop after approx. 10 seconds.\e[0m "
echo                 "################################################################"
echo

ceps_binary_alternative1="../../bin/ceps"
ceps_binary_alternative2="ceps"
ceps_binary="$ceps_binary_alternative1"
ws_api_port="8182" 
ceps_server_pid=""
nodejs_binary="node"
node_js_pid=""

function check_ceps_binary () {
	($ceps_binary --version > /dev/null 2> /dev/null)
       	r=$?
	if [ $r -ne 0 ] 
	then
		ceps_binary="$ceps_binary_alternative2"
		($ceps_binary --version > /dev/null 2> /dev/null)
		r=$?
		if [ $r -ne 0 ] 
		then
			echo "****Fatal error: cannot find ceps. Tried \"${ceps_binary_alternative1}\",\"${ceps_binary_alternative2}\"."
			exit 1
		fi
	fi	
}

function launch_ceps_as_server () {
 local wsapi=$1
 shift 1
 local files=$@
 ${ceps_binary} $files "--ws_api" "$wsapi" &
 ceps_server_pid=$!
}

function launch_nodejs_server () {
 local args=$@
 cd nodejs
 ${nodejs_binary} $args &
 node_js_pid=$!
 cd ..
}

function cleanup()
{
	kill "$ceps_server_pid" > /dev/null 2> /dev/null
	kill "$node_js_pid" > /dev/null 2> /dev/null
 	exit 1
}

trap cleanup SIGINT
trap cleanup EXIT


check_ceps_binary
echo -e "\e[38;5;105mLaunching ceps as WebSocket server listening on port ${ws_api_port}.\e[0m "
launch_ceps_as_server "$ws_api_port" "a_simple_statemachine.ceps"
if [[ "$ceps_server_pid" == "" ]]; then
	echo -e "\e[38;5;196m****Fatal error: Failed to run ceps as WebSocket server listening on port ${ws_api_port}.\e[0m"
	exit 1
fi
sleep 2

a=$( ps "--pid" "$ceps_server_pid" | grep "$ceps_server_pid" -c )
if [[ ">$a<" != ">1<" ]]; then
	echo -e "\e[38;5;196m****Fatal error: ceps server exited unexpectedly (port ${ws_api_port} already in use?).\e[0m"
 	exit 1
fi

echo -e "\e[38;5;105mLaunching client in nodejs/index.js.\e[0m "
launch_nodejs_server "index.js"
sleep 5

a=$( ps "--pid" "$node_js_pid" | grep "$node_js_pid" -c )
if [[ ">$a<" != ">1<" ]]; then
	echo -e "\e[38;5;196m****Fatal error: Node.js unexpectedly terminated/failed to run (Is node (node.js) installed? Did you run npm install inside the nodejs directory?).\e[0m"
 	exit 1
fi


ctr=0
for (( ; ; ))
do
 sleep 1
 ((ctr=ctr + 1))
 if [ $ctr -ge 10 ]
 then
	break
 fi
 a=$( ps "--pid" "$ceps_server_pid" | grep "$ceps_server_pid" -c )
 if [[ ">$a<" != ">1<" ]]; then
  break
 fi
done
echo
echo -e "\e[38;5;40mTest successfully terminated.\e[0m"
echo