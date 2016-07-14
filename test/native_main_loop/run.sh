#!/bin/bash
echo -e "\e[104mRunning sm4ceps tests: Native Main Loop\e[0m"
echo


echo -en "Test \e[4mmoved_transitions\e[0m:"
./run_moved_transitions.sh > moved_transitions/log.txt
if [ $? -ne 0 ]; then
  echo -e "\e[38;5;196m failed"
  echo "..."
  tail  moved_transitions/log.txt --lines=25
  echo -e "\e[0m"
else
 diff moved_transitions/log.txt moved_transitions/expected_log.txt > moved_transitions/diff.log
 if [ $? -ne 0 ]; then
   echo -e "\e[38;5;196m failed"
   cat moved_transitions/diff.log
   echo -e "\e[0m"
 else
   echo -e "\e[38;5;28m passed \e[0m"
  fi
fi


echo -en "Test \e[4msubstatemachines_basic_a\e[0m:"
./run_substatemachines_basic_a.sh > substatemachines_basic_a/log.txt
if [ $? -ne 0 ]; then
  echo -e "\e[38;5;196m failed"
  echo "..."
  tail  substatemachines_basic_a/log.txt --lines=25
  echo -e "\e[0m"
else
 diff substatemachines_basic_a/log.txt substatemachines_basic_a/expected_log.txt > substatemachines_basic_a/diff.log
 if [ $? -ne 0 ]; then
   echo -e "\e[38;5;196m failed"
   cat substatemachines_basic_a/diff.log
   echo -e "\e[0m"
 else
   echo -e "\e[38;5;28m passed \e[0m"
  fi
fi
 
echo -en "Test \e[4mtimer_a\e[0m:"
./run_timer_a.sh > timer_a/log.txt
if [ $? -ne 0 ]; then
  echo -e "\e[38;5;196m failed"
  echo "..."
  tail  timer_a/log.txt --lines=25
  echo -e "\e[0m"
else
 diff timer_a/log.txt timer_a/expected_log.txt > timer_a/diff.log
 if [ $? -ne 0 ]; then
   echo -e "\e[38;5;196m failed"
   cat timer_a/diff.log
   echo -e "\e[0m"
 else
   echo -e "\e[38;5;28m passed \e[0m"
  fi
fi
