../../x86/sm send_counter.ceps --cppgen --ignore_simulations > /dev/null
mv out.* send_counter/
g++ -std=c++1y -fPIC -shared send_counter/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"send_counter/" -I"../../" -o send_counter/lib.so
../../x86/sm send_counter.ceps --rip127.0.0.1 --rport2001 --server --port2003 --enforce_native --plugin./send_counter/lib.so --quiet

