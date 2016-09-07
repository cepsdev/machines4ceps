echo "Generate code"
../../x86/sm substatemachines_basic_a.ceps --cppgen --ignore_simulations --cppgen_statemachines
mv out.* substatemachines_basic_a/
echo "Compile"
g++ -std=c++1y -fPIC -shared substatemachines_basic_a/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"substatemachines_basic_a/" -I"../../" -o substatemachines_basic_a/lib.so
echo "Run"
# ../../x86/sm substatemachines_basic_a.ceps --enforce_native --plugin./substatemachines_basic_a/lib.so
