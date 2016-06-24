#!/bin/bash


###############################################################
##      constants
###############################################################
interfaces=(vcan0 vcan1 can0 can1)
praefix="./"
suffix="_config.sh 2> /dev/null"

###############################################################
##      functions
###############################################################
function feedback 
{
    if [ $1 -eq 0 ]
    then
    echo -e "\t[OK]"
    else
    echo -e "\t[FAILED]" >&2
    fi
}

###############################################################
echo -e "Going to init all interfaces:"
for i in ${interfaces[@]}; do
    echo -e "$i ..\c"
    $praefix$i$suffix
    feedback $?
done
exit 0