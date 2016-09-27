echo "Generate code(1/2)"
../../x86/sm substatemachines_basic_a.ceps --cppgen --ignore_simulations #--cppgen_statemachines
cp out.cpp substatemachines_basic_a/out_cppgen_sm.cpp
mv out* substatemachines_basic_a/
echo "Compile(1/2)"
g++ -std=c++1y -fPIC -shared substatemachines_basic_a/out.cpp substatemachines_basic_a/out_create_statemachines.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"substatemachines_basic_a/" -I"../../" -o substatemachines_basic_a/lib.so
echo "Run(1/2)"
../../x86/sm substatemachines_basic_a.ceps --enforce_native --live_log --plugin./substatemachines_basic_a/lib.so
#echo "Generate code(2/2)"
#../../x86/sm substatemachines_basic_a.ceps --cppgen --ignore_simulations
#mv out.* substatemachines_basic_a/
#echo "Compile(2/2)"
#g++ -std=c++1y -fPIC -shared substatemachines_basic_a/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"substatemachines_basic_a/" -I"../../" -o #substatemachines_basic_a/lib.so
#echo "Run(2/2)"
#../../x86/sm substatemachines_basic_a.ceps --enforce_native --plugin./substatemachines_basic_a/lib.so

 
