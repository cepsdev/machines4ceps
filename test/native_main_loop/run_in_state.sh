../../x86/sm in_state.ceps --cppgen > /dev/null
mv out.* in_state/
g++ -std=c++1y -fPIC -shared in_state/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"in_state/" -I"../../" -o in_state/lib.so
../../x86/sm in_state.ceps --enforce_native --plugin./in_state/lib.so
