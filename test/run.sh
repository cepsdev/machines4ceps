#!/bin/bash
echo -e "\e[104mRunning sm4ceps tests\e[0m"
echo

 echo -en "Test \e[4mGeneration of C++ code\e[0m (basic examples - basic_example_1) :"
 cd cppgen/basic_examples
 rm out.cpp
 rm out.hpp
 ../../../x86/sm basic_example_1.ceps --cppgen --quiet > basic_example_1_cppgen_step.log 2> basic_example_1_cppgen_step.log
 if [ $? -ne 0 ]; then
  echo -e "\e[38;5;196m failed (cpp generation step)"
  cat basic_example_1_cppgen_step.log
  echo -e "\e[0m"
 else
  g++ -fPIC -shared -std=c++11 out.cpp -I"../" -I"../../../" -o basic_example_1.so > basic_example_1_buildso_step.log 2> basic_example_1_buildso_step.log
  if [ $? -ne 0 ]; then
   echo -e "\e[38;5;196m failed (build shared library step)"
   cat basic_example_1_buildso_step.log
   echo -e "\e[0m"
  else
   ../../../x86/sm basic_example_1_actions_removed.ceps --plugin./basic_example_1.so --quiet > basic_example_1_actions_removed_step.log 2> basic_example_1_actions_removed_step.log
   if [ $? -ne 0 ]; then
    echo -e "\e[38;5;196m failed (Rerunning basic_example_1 with native actions)"
    cat basic_example_1_actions_removed_step.log
    echo -e "\e[0m"
   else
    diff basic_example_1_actions_removed_step.log basic_example_1_cppgen_step.log >diff.log 2>diff.log
    if [ $? -ne 0 ]; then
     echo -e "\e[38;5;196m failed (Output differs between pure interpreted run and run with actions replaced by native code)"
     cat diff.log
     echo -e "\e[0m"
    else
     echo -e "\e[38;5;28m passed \e[0m"
    fi
   fi 
  fi
 fi
 cd ../..


 echo -en "Test \e[4mGeneration of C++ code\e[0m (basic examples - basic_example_2) :"
 cd cppgen/basic_examples
 rm out.cpp
 rm out.hpp
 ../../../x86/sm basic_example_2.ceps --cppgen --quiet > basic_example_2_cppgen_step.log 2> basic_example_2_cppgen_step.log
 if [ $? -ne 0 ]; then
  echo -e "\e[38;5;196m failed (cpp generation step)"
  cat basic_example_2_cppgen_step.log
  echo -e "\e[0m"
 else
  cp out.cpp basic_example_2.cpp
  cp out.hpp basic_example_2.hpp
  g++ -fPIC -shared -std=c++11 out.cpp -I"../" -I"../../../" -o basic_example_2.so > basic_example_2_buildso_step.log 2> basic_example_2_buildso_step.log
  if [ $? -ne 0 ]; then
   echo -e "\e[38;5;196m failed (build shared library step)"
   echo
   cat basic_example_2_buildso_step.log
   echo -e "\e[0m"
  else
   ../../../x86/sm basic_example_2_ripped.ceps --plugin./basic_example_2.so --quiet > basic_example_2_actions_removed_step.log 2> basic_example_2_actions_removed_step.log
   if [ $? -ne 0 ]; then
    echo -e "\e[38;5;196m failed (Rerunning basic_example_2 with native actions)"
    echo
    cat basic_example_2_actions_removed_step.log
    echo -e "\e[0m"
   else
    diff basic_example_2_actions_removed_step.log basic_example_2_cppgen_step.log >diff.log 2>diff.log
    if [ $? -ne 0 ]; then
     echo -e "\e[38;5;196m failed (Output differs between pure interpreted run and run with actions replaced by native code)"
     echo
     cat diff.log
     echo -e "\e[0m"
    else
     echo -e "\e[38;5;28m passed \e[0m"
    fi
   fi 
  fi
 fi
 cd ../..

echo -en "Test \e[4mGeneration of C++ code\e[0m (basic examples - basic_example_3) :"
 cd cppgen/basic_examples
 rm out.cpp
 rm out.hpp
 ../../../x86/sm basic_example_3.ceps --cppgen --quiet > basic_example_3_cppgen_step.log 2> basic_example_3_cppgen_step.log
 cp out.cpp basic_example_3_out.cpp
 cp out.hpp basic_example_3_out.hpp
 if [ $? -ne 0 ]; then
  echo -e "\e[38;5;196m failed (cpp generation step)"
  cat basic_example_3_cppgen_step.log
  echo -e "\e[0m"
 else
  g++ -fPIC -shared -std=c++11 out.cpp -I"../" -I"../../../" -o basic_example_3.so > basic_example_3_buildso_step.log 2> basic_example_3_buildso_step.log
  if [ $? -ne 0 ]; then
   echo -e "\e[38;5;196m failed (build shared library step)"
   echo
   cat basic_example_3_buildso_step.log
   echo -e "\e[0m"
  else
   ../../../x86/sm basic_example_3_ripped.ceps --plugin./basic_example_3.so --quiet > basic_example_3_actions_removed_step.log 2> basic_example_3_actions_removed_step.log
   if [ $? -ne 0 ]; then
    echo -e "\e[38;5;196m failed (Rerunning basic_example_3 with native actions)"
    echo
    cat basic_example_3_actions_removed_step.log
    echo -e "\e[0m"
   else
    diff basic_example_3_actions_removed_step.log basic_example_3_cppgen_step.log >diff.log 2>diff.log
    if [ $? -ne 0 ]; then
     echo -e "\e[38;5;196m failed (Output differs between pure interpreted run and run with actions replaced by native code)"
     echo
     cat diff.log
     echo -e "\e[0m"
    else
     echo -e "\e[38;5;28m passed \e[0m"
    fi
   fi 
  fi
 fi
 cd ../..


echo -en "Test \e[4mGeneration of C++ code\e[0m (basic examples - basic_example_4) :"
 cd cppgen/basic_examples
 rm out.cpp
 rm out.hpp
 ../../../x86/sm basic_example_4.ceps --cppgen --quiet > basic_example_4_cppgen_step.log 2> basic_example_4_cppgen_step.log
 cp out.cpp basic_example_4_out.cpp
 cp out.hpp basic_example_4_out.hpp
 if [ $? -ne 0 ]; then
  echo -e "\e[38;5;196m failed (cpp generation step)"
  cat basic_example_4_cppgen_step.log
  echo -e "\e[0m"
 else
  g++ -fPIC -shared -std=c++11 out.cpp -I"../" -I"../../../" -o basic_example_4.so > basic_example_4_buildso_step.log 2> basic_example_4_buildso_step.log
  if [ $? -ne 0 ]; then
   echo -e "\e[38;5;196m failed (build shared library step)"
   echo
   cat basic_example_4_buildso_step.log
   echo -e "\e[0m"
  else
   ../../../x86/sm basic_example_4_ripped.ceps --plugin./basic_example_4.so --quiet > basic_example_4_actions_removed_step.log 2> basic_example_4_actions_removed_step.log
   if [ $? -ne 0 ]; then
    echo -e "\e[38;5;196m failed (Rerunning basic_example_4 with native actions)"
    echo
    cat basic_example_4_actions_removed_step.log
    echo -e "\e[0m"
   else
    diff basic_example_4_actions_removed_step.log basic_example_4_cppgen_step.log >diff.log 2>diff.log
    if [ $? -ne 0 ]; then
     echo -e "\e[38;5;196m failed (Output differs between pure interpreted run and run with actions replaced by native code)"
     echo
     cat diff.log
     echo -e "\e[0m"
    else
     echo -e "\e[38;5;28m passed \e[0m"
    fi
   fi 
  fi
 fi
 cd ../..
 
echo -en "Test \e[4msystemparameter.ceps\e[0m                                      :"
../x86/sm systemparameter.ceps --quiet >systemparameter.log 2>systemparameter.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mthreads1.ceps\e[0m                                             :"
../x86/sm threads1.ceps --quiet >threads1.log 2>>threads1.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mplatform_extend_fto_and_alu.ceps\e[0m                          :"
../x86/sm platform_extend_fto_and_alu.ceps --quiet >platform_extend_fto_and_alu.log 2>platform_extend_fto_and_alu.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 cat platform_extend_fto_and_alu.log
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mepsilon_transitions1.ceps\e[0m                                 :"
../x86/sm epsilon_transitions1.ceps --quiet >epsilon_transitions1.log 2>epsilon_transitions1.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 cat epsilon_transitions1.log
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi


echo -en "Test \e[4mconditionals1.ceps\e[0m                                        :"
../x86/sm conditionals1.ceps --quiet >conditionals1.log 2>conditionals1.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 cat conditionals1.log
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mon_enter_semantics.ceps\e[0m                                   :"
../x86/sm on_enter_semantics.ceps --quiet >on_enter_semantics.log 2>on_enter_semantics.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mdistributed_native_1_single_simulation_core\e[0m               :"
../x86/sm distributed_native_1_node_a.ceps distributed_native_1_node_b.ceps simulation_distributed_native_1_run_both_nodes_on_one_sim_core.ceps >distributed_native_1_single_simulation_core.log 2>distributed_native_1_single_simulation_core.log
if [ $? -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 cat distributed_native_1_single_simulation_core.log
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mdistributed_native_1_two_simulation_cores\e[0m                 :"

../x86/sm distributed_native_1_node_a.ceps --server --rip127.0.0.1 --port4001 --rport4002\
 simulation_distributed_native_1_run_node_a.ceps >simulation_distributed_native_1_run_node_a.log 2>simulation_distributed_native_1_run_node_a.log &
my_pid1=$!
../x86/sm distributed_native_1_node_b.ceps --server --rip127.0.0.1 --port4002 --rport4001\
 simulation_distributed_native_1_run_node_b.ceps >simulation_distributed_native_1_run_node_b.log 2>simulation_distributed_native_1_run_node_b.log &
my_pid2=$!
#$(sleep 5;kill $my_pid1;kill $my_pid2) &
wait $my_pid1 
exit_status_1=$?
wait $my_pid2 
exit_status_2=$?
if [ $exit_status_1 -ne 0 ] || [ $exit_status_2 -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 if [ $exit_status_1 -ne 0 ]; then
  cat simulation_distributed_native_1_run_node_a.log
 fi
 if [ $exit_status_2 -ne 0 ]; then
  cat simulation_distributed_native_1_run_node_b.log
 fi
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mdistributed_raw_frame_1_two_simulation_cores\e[0m              :"

../x86/sm distributed_native_1_node_a.ceps \
 distributed_raw_frame_1_message_definition.ceps channels_node_a_rawframe.ceps simulation_distributed_native_1_run_node_a.ceps >distributed_raw_frame_1_one_simulation_core_node_a.log 2>distributed_raw_frame_1_one_simulation_core_node_a.log &
my_pid1=$!
../x86/sm distributed_native_1_node_b.ceps \
 distributed_raw_frame_1_message_definition.ceps channels_node_b_rawframe.ceps simulation_distributed_native_1_run_node_b.ceps >distributed_raw_frame_1_one_simulation_core_node_b.log 2>distributed_raw_frame_1_one_simulation_core_node_b.log &
my_pid2=$!
#$(sleep 5;kill $my_pid1;kill $my_pid2) &
wait $my_pid1 
exit_status_1=$?
wait $my_pid2 
exit_status_2=$?
if [ $exit_status_1 -ne 0 ] || [ $exit_status_2 -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 if [ $exit_status_1 -ne 0 ]; then
  cat distributed_raw_frame_1_one_simulation_core_node_a.log
 fi
 if [ $exit_status_2 -ne 0 ]; then
  cat distributed_raw_frame_1_one_simulation_core_node_b.log
 fi
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi

echo -en "Test \e[4mdistr_via_canbus_loopback_1_two_simulation_cores\e[0m          :"

../x86/sm distributed_native_1_node_b.ceps \
 distributed_raw_frame_1_message_definition.ceps channels_node_b_rawframe_canbus_loopback.ceps simulation_distributed_native_1_run_node_b.ceps >distributed_raw_frame_1_one_simulation_core_node_b_canbus.log 2>distributed_raw_frame_1_one_simulation_core_node_b_canbus.log &
my_pid2=$!
sleep 2
../x86/sm distributed_native_1_node_a.ceps distributed_raw_frame_1_message_definition.ceps channels_node_a_rawframe_canbus_loopback.ceps simulation_distributed_native_1_run_node_a.ceps >distributed_raw_frame_1_one_simulation_core_node_a_canbus.log 2>distributed_raw_frame_1_one_simulation_core_node_a_canbus.log &
my_pid1=$!

#$(sleep 5;kill $my_pid1;kill $my_pid2) &
wait $my_pid1 
exit_status_1=$?
wait $my_pid2 
exit_status_2=$?
if [ $exit_status_1 -ne 0 ] || [ $exit_status_2 -ne 0 ]; then
 echo -e "\e[38;5;196m failed \e[0m"
 echo -e "\e[38;5;196m"
 if [ $exit_status_1 -ne 0 ]; then
  cat distributed_raw_frame_1_one_simulation_core_node_a_canbus.log
 fi
 if [ $exit_status_2 -ne 0 ]; then
  cat distributed_raw_frame_1_one_simulation_core_node_b_canbus.log
 fi
 echo -e "\e[0m"
else
 echo -e "\e[38;5;28m passed \e[0m"
fi


# make clean > /dev/null 2> /dev/null
# make > plugin_support_make_step.log 2> plugin_support_make_step.log
# if [ $? -ne 0 ]; then
#  echo -e "\e[38;5;196m failed (build step)"
#  cat plugin_support_make_step.log
#  echo -e "\e[0m"
# else
#   ../../x86/sm example.ceps --quiet --pluginAntriebe.so > plugin_support_run_step.log 2> plugin_support_run_step.log
#  if [ $? -ne 0 ]; then
#   echo -e "\e[38;5;196m failed \e[0m"
#  else
#   echo -e "\e[38;5;28m passed \e[0m"
#  fi
# fi
# 

# echo -en "Test \e[4mplugin_support\e[0m                                     :"
# cd plugin_sample_1
# make clean > /dev/null 2> /dev/null
# make > plugin_support_make_step.log 2> plugin_support_make_step.log
# if [ $? -ne 0 ]; then
#  echo -e "\e[38;5;196m failed (build step)"
#  cat plugin_support_make_step.log
#  echo -e "\e[0m"
# else
#   ../../x86/sm example.ceps --quiet --pluginAntriebe.so > plugin_support_run_step.log 2> plugin_support_run_step.log
#  if [ $? -ne 0 ]; then
#   echo -e "\e[38;5;196m failed \e[0m"
#  else
#   echo -e "\e[38;5;28m passed \e[0m"
#  fi
# fi
# 
# cd ..


