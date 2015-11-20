#!/bin/bash
echo -e "\e[104mRunning sm4ceps tests\e[0m"
echo 
echo -en "Test \e[4msystemparameter.ceps\e[0m                 :"
../x86/sm systemparameter.ceps --quiet >systemparameter.log 2>systemparameter.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mthreads1.ceps\e[0m                        :"
../x86/sm threads1.ceps --quiet >threads1.log 2>threads1.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mplatform_extend_fto_and_alu.ceps\e[0m     :"
../x86/sm platform_extend_fto_and_alu.ceps --quiet >platform_extend_fto_and_alu.log 2>platform_extend_fto_and_alu.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 cat platform_extend_fto_and_alu.log
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mepsilon_transitions1.ceps\e[0m            :"
../x86/sm epsilon_transitions1.ceps --quiet >epsilon_transitions1.log 2>epsilon_transitions1.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 cat epsilon_transitions1.log
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi


echo -en "Test \e[4mconditionals1.ceps\e[0m                   :"
../x86/sm conditionals1.ceps --quiet >conditionals1.log 2>conditionals1.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 cat conditionals1.log
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi
