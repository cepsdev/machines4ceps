#!/bin/bash
echo -e "\e[104mRunning sm4ceps tests\e[0m"
echo 
echo -en "Test \e[4msystemparameter.ceps\e[0m:"
../x86/sm systemparameter.ceps --quiet >systemparameter.log 2>systemparameter.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi



