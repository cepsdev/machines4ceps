../../x86/sm xml_msgs.ceps  xml_node_a.ceps --cppgen --ignore_simulations 
mv out* xml_node_a/
g++ -std=c++1y -fPIC -shared xml_node_a/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"xml_node_a/" -I"../../" -o xml_node_a/lib.so
../../x86/sm xml_msgs.ceps  xml_node_a.ceps --enforce_native --plugin./xml_node_a/lib.so 
