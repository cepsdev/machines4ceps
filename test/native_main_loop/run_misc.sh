../../x86/sm misc.ceps --cppgen --ignore_simulations  > /dev/null
mv out* misc/
g++ -std=c++1y -fPIC -shared misc/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"misc/" -I"../../" -o misc/lib.so
../../x86/sm misc.ceps  --enforce_native --plugin./misc/lib.so
