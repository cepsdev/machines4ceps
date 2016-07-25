../../x86/sm ev_with_payload.ceps --cppgen --ignore_simulations  > /dev/null
mv out* ev_with_payload/
g++ -std=c++1y -fPIC -shared ev_with_payload/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"ev_with_payload/" -I"../../" -o ev_with_payload/lib.so
../../x86/sm ev_with_payload.ceps --enforce_native --plugin./ev_with_payload/lib.so
