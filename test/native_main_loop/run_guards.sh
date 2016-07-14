../../x86/sm guards.ceps --cppgen > /dev/null
mv out.* guards/
g++ -std=c++1y -fPIC -shared guards/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"guards/" -I"../../" -o guards/lib.so
../../x86/sm guards.ceps --enforce_native --plugin./guards/lib.so
