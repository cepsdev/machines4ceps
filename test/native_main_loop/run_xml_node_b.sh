../../x86/sm xml_msgs.ceps  xml_node_b.ceps --cppgen --ignore_simulations  > /dev/null
mv out* xml_node_b/
g++ -std=c++1y -fPIC -shared xml_node_b/out.cpp ../../core/src/state_machine_simulation_core_plugin_interface.cpp -I"xml_node_b/" -I"../../" -o xml_node_b/lib.so
../../x86/sm xml_msgs.ceps  xml_node_b.ceps --enforce_native --plugin./xml_node_b/lib.so
