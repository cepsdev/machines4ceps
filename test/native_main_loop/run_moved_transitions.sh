../../x86/sm moved_transitions.ceps --cppgen > /dev/null
mv out.* moved_transitions/
g++ -std=c++1y -fPIC -shared moved_transitions/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"moved_transitions/" -I"../../" -o moved_transitions/lib.so
../../x86/sm moved_transitions.ceps --enforce_native --plugin./moved_transitions/lib.so
