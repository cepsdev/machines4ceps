../../x86/sm receive_counter.ceps --cppgen --ignore_simulations > /dev/null
mv out.* receive_counter/
g++ -std=c++1y -fPIC -shared receive_counter/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"receive_counter/" -I"../../" -o receive_counter/lib.so
../../x86/sm receive_counter.ceps --enforce_native --plugin./receive_counter/lib.so --rip127.0.0.1 --rport2003 --server --port2001 --quiet
