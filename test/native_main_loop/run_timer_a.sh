../../x86/sm timer_a.ceps --cppgen --ignore_simulations  > /dev/null
mv out.* timer_a/
g++ -std=c++1y -fPIC -shared timer_a/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"timer_a/" -I"../../" -o timer_a/lib.so
../../x86/sm timer_a.ceps  --enforce_native --plugin./timer_a/lib.so
