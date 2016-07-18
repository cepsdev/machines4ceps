../../x86/sm timer_b.ceps --cppgen --ignore_simulations  > /dev/null
mv out.* timer_b/
g++ -std=c++1y -fPIC -shared timer_b/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"timer_b/" -I"../../" -o timer_b/lib.so
../../x86/sm timer_b.ceps  --enforce_native --plugin./timer_b/lib.so
