#include "fibex_import.hpp"

#define INVARIANT(x)

struct fibex_coding{
  std::string base_data_type;
  std::string short_name;
  std::string category;
  std::string encoding;
  std::string bit_length;

  double min = 0;
  double max = 0;
  double internal_to_phs_scale = 1.0;
  double internal_to_phs_offset = 0.0;

  std::string ceps_encoding_type;
};


struct fibex_signal{
	 static std::string normalize_signal_name(std::string);
	 std::string name;
	 double default_value=0;
	 std::string sid;
	 fibex_coding encoding;
	 std::string cod_ref;
	 fibex_signal(std::vector<ceps::ast::Nodebase_ptr> const & v){
      for(auto e : v){
    	  if (e->kind() != ceps::ast::Ast_node_kind::structdef) continue;
    	  auto & s = ceps::ast::as_struct_ref(e);
    	  auto s_name = ceps::ast::name(s);
    	  if ("ho:SHORT-NAME" == s_name)
    		name = normalize_signal_name(ceps::ast::Nodeset(e)["ho:SHORT-NAME"].as_str());
    	  else if ("fx:DEFAULT-VALUE" == s_name)
    		  default_value = std::stod(ceps::ast::Nodeset(e)["fx:DEFAULT-VALUE"].as_str());
    	  else if ("@ID" == s_name){
    		  sid = ceps::ast::Nodeset(e)["@ID"].as_str();
    	  } else if ("fx:CODING-REF" == s_name){
              cod_ref = ceps::ast::Nodeset(e)["fx:CODING-REF"]["@ID-REF"].as_str();
    	  }
      }//for
	 }
 };


 struct fibex_pdu{
	 std::string short_name;
	 std::string desc;
	 std::string type;
	 std::string id;
	 unsigned short byte_length=0;

	 struct signal_instance{
		 unsigned short bitposition = 0;
		 bool valid = true;
		 bool bigendian = false;
		 std::string sigid;
		 size_t sigidx = 0;
	 };

	 std::vector<signal_instance> sig_instances;

	 fibex_pdu(std::vector<ceps::ast::Nodebase_ptr> const & v){
      for(auto e : v){
    	  if (e->kind() != ceps::ast::Ast_node_kind::structdef) continue;
    	  auto & s = ceps::ast::as_struct_ref(e);
    	  if (s.children().size() == 0) continue;

    	  auto s_name = ceps::ast::name(s);
    	  //std::cout << *e << std::endl;
    	  if ("@ID" == s_name)
    		  id = ceps::ast::Nodeset(e)["@ID"].as_str();
    	  else if ("ho:SHORT-NAME" == s_name)
     		 short_name = ceps::ast::Nodeset(e)["ho:SHORT-NAME"].as_str();
     	  else if ("ho:DESC" == s_name)
      		 desc = ceps::ast::Nodeset(e)["ho:DESC"].as_str();
     	  else if ("fx:PDU-TYPE" == s_name)
      		 type = ceps::ast::Nodeset(e)["fx:PDU-TYPE"].as_str();
     	  else if ("fx:BYTE-LENGTH" == s_name)
      		 byte_length = std::stoi(ceps::ast::Nodeset(e)["fx:BYTE-LENGTH"].as_str());
     	 else if ("fx:SIGNAL-INSTANCES" == s_name){
    		 auto s_instances = ceps::ast::Nodeset(e)["fx:SIGNAL-INSTANCES"][ceps::ast::all{"fx:SIGNAL-INSTANCE"}];
    		 for(auto e : s_instances.nodes()){
    			 auto s_instance = ceps::ast::Nodeset(e)["fx:SIGNAL-INSTANCE"];
                 signal_instance s;
                 s.sigid = s_instance["fx:SIGNAL-REF"]["@ID-REF"].as_str();
                 s.bitposition = std::stoi(s_instance["fx:BIT-POSITION"].as_str());
                 s.bigendian = s_instance["fx:IS-HIGH-LOW-BYTE-ORDER"].as_str() == "true";
                 sig_instances.push_back(s);
    		 }
     	 }
      }//for
      std::sort(sig_instances.begin(), sig_instances.end(), [](signal_instance const & a, signal_instance const & b) { return a.bitposition < b.bitposition;} );
	 }//fibex_pdu ctor
 };//fibex_pdu


 struct fibex_frame{
	 std::string short_name;
	 std::string id;
	 std::string desc;
	 std::vector<std::pair<fibex_pdu,size_t>> pdus;
 };

 std::string fibex_signal::normalize_signal_name(std::string s){
  std::string::size_type j = 0;
  for(;j!=s.length();++j){
	  if (std::isalpha(s[j])) break;
  }
  if (j == s.length()) return "";
  return s.substr(j);
 }

 ceps::ast::Nodeset sm4ceps::utils::import_fibex(
		        State_machine_simulation_core* smc,
  				ceps::ast::Struct_ptr fibex_struct,
  				ceps::parser_env::Symboltable & sym_tab,
  				ceps::interpreter::Environment& env 
  				){

  static std::map<std::string,std::string> cod2internal_cod {  {"A_UINT8","uint"},{"A_UINT16","uint"},{"A_UINT32","uint"},{"A_UINT64","uint"},{"A_UINT4","uint"},
	                                                           {"A_INT8","int"},{"A_INT16","int"},{"A_INT32","int"},{"A_INT64","int"},{"A_INT4","int"}
                                             };
  ceps::ast::Nodeset r;
  ceps::ast::Nodeset ns{fibex_struct->children()};
  std::map<std::string,fibex_coding> encodings_map;
  std::map<std::string,std::string> frm_id_to_frame_name;

  auto encodings = ns["fx:PROCESSING-INFORMATION"]["fx:CODINGS"][ceps::ast::all{"fx:CODING"}];
  for (auto const & e_ : encodings){
	auto e = e_["fx:CODING"];
    fibex_coding cod;
    cod.base_data_type = e["ho:CODED-TYPE"]["@ho:BASE-DATA-TYPE"].as_str();
    cod.short_name = e["ho:SHORT-NAME"].as_str();
    cod.category = e["ho:CODED-TYPE"]["@CATEGORY"].as_str();
    cod.encoding = e ["ho:CODED-TYPE"]["@ENCODING"].as_str();
    cod.bit_length = e["ho:CODED-TYPE"]["ho:BIT-LENGTH"].as_str();
    if (cod.bit_length == "1")
     cod.ceps_encoding_type = "bool";
    else{
     if (cod.encoding == "UNSIGNED")
    	 cod.ceps_encoding_type = "uint"; else cod.ceps_encoding_type = "int";
     cod.ceps_encoding_type += cod.bit_length;cod.ceps_encoding_type += "_t";
    }

    for(auto f : e["ho:COMPU-METHODS"][ceps::ast::all{"ho:COMPU-METHOD"}]){
    	auto compu_method = f["ho:COMPU-METHOD"];
    	cod.min = std::stod(compu_method["ho:PHYS-CONSTRS"]["ho:SCALE-CONSTR"]["ho:LOWER-LIMIT"].as_str());
    	cod.max = std::stod(compu_method["ho:PHYS-CONSTRS"]["ho:SCALE-CONSTR"]["ho:UPPER-LIMIT"].as_str());
    	auto scale = compu_method["ho:COMPU-INTERNAL-TO-PHYS"]["ho:COMPU-SCALES"]["ho:COMPU-SCALE"]["ho:COMPU-RATIONAL-COEFFS"]["ho:COMPU-NUMERATOR"][ceps::ast::all{"ho:V"}];
    	cod.internal_to_phs_offset = std::stod(scale[ceps::ast::nth{0}].as_str());
    	cod.internal_to_phs_scale = std::stod(scale[ceps::ast::nth{1}].as_str());
    }


    encodings_map[e["@ID"].as_str()] = cod;
  }

  auto signals = ns["fx:ELEMENTS"]["fx:SIGNALS"];
  std::vector<fibex_signal> sig_vec;
  std::vector<fibex_pdu> pdu_vec;


  for(auto e : signals.nodes()){
	  if (e->kind() != ceps::ast::Ast_node_kind::structdef || ceps::ast::name(ceps::ast::as_struct_ref(e)) != "fx:SIGNAL") continue;
	  sig_vec.push_back(fibex_signal(ceps::ast::as_struct_ref(e).children()));

  }

  std::map<std::string,size_t> sigvec_id2idx;
  auto sysstate_symbol = sym_tab.lookup("Systemstate",false,false,false);
  if (sysstate_symbol == nullptr)
  {
	sysstate_symbol = sym_tab.lookup("Systemstate",true,false,false);
	sysstate_symbol->category = ceps::parser_env::Symbol::KIND;
  }

  for(size_t i = 0; i != sig_vec.size(); ++i){
	auto new_symbol = sym_tab.lookup(sig_vec[i].name,true,false,false);
    new_symbol->payload = sysstate_symbol;
    new_symbol->category = ceps::parser_env::Symbol::SYMBOL;
	sigvec_id2idx[sig_vec[i].sid] = i;
	auto it = encodings_map.find(sig_vec[i].cod_ref);
	if (encodings_map.end() == it)
		smc->warn_(-1,"Import of FIBEX data: no encoding found with id '"+sig_vec[i].cod_ref+"' as referenced by signal instance with name '"+sig_vec[i].name+"'");
	else sig_vec[i].encoding = it->second;
  }

  auto glob_sec = new ceps::ast::Struct("Globals",nullptr,nullptr,nullptr);

  for(auto const & s:sig_vec){
	  glob_sec->children().push_back(
	   new ceps::ast::Binary_operator('=',
			                       new ceps::ast::Symbol(s.name,"Systemstate"),
								   new ceps::ast::Double(s.default_value,ceps::ast::all_zero_unit()) )
	  );
  }

  r.nodes().push_back(glob_sec);

  INVARIANT(signals processed and stored in order of appearance in sig_vec)
  INVARIANT(sigvec_id2idx maps xml-id of individual signal to its position in sig_vec)
  INVARIANT(Globals - section with default initializer complete and pushed to result node set)

  auto pdus = ns["fx:ELEMENTS"]["fx:PDUS"];

  for(auto e : pdus.nodes()){
	  if (e->kind() != ceps::ast::Ast_node_kind::structdef || ceps::ast::name(ceps::ast::as_struct_ref(e)) != "fx:PDU") continue;
      pdu_vec.push_back(fibex_pdu(ceps::ast::as_struct_ref(e).children()));
  }

  for(auto& e : pdu_vec){
	  for(auto& ee : e.sig_instances){
		  auto it = sigvec_id2idx.find(ee.sigid);
		  if (it == sigvec_id2idx.end()) {
			  smc->warn_(-1,"Import of FIBEX data: no signal definition found with id '"+ee.sigid+"' as referenced by signal instance with short name '"+e.short_name+"'");
			  ee.valid = false;
			  continue;
		  }
		  ee.sigidx = it->second;
	  }
  }

  INVARIANT(PDUs stored in pdu_vec and signal instances with valid-attribute set to true are correctly wired up to corresponding signal in sig_vec)

  auto frames = ns["fx:ELEMENTS"]["fx:FRAMES"][ceps::ast::all{"fx:FRAME"}];
  std::vector<fibex_frame> fibex_frames;
  for (auto e : frames.nodes()){
	  auto frm = ceps::ast::Nodeset(e)["fx:FRAME"];
	  fibex_frame frame;
	  frame.short_name = frm["ho:SHORT-NAME"].as_str();
	  frame.id = frm["@ID"].as_str();
	  frm_id_to_frame_name[frame.id] = frame.short_name;

	  frame.desc = frm["ho:DESC"].as_str();
	  auto pdus = frm["fx:PDU-INSTANCES"][ceps::ast::all{"fx:PDU-INSTANCE"}];
	  for (auto ee : pdus.nodes() ){
		  auto pdu_inst = ceps::ast::Nodeset(ee)["fx:PDU-INSTANCE"];
		  auto pdu_inst_id = pdu_inst["fx:PDU-REF"]["@ID-REF"].as_str();
		  size_t bit_pos = std::stoi(pdu_inst["fx:BIT-POSITION"].as_str());
		  bool found = false;
		  for (auto const & pdu : pdu_vec){
            if (pdu.id != pdu_inst_id) continue;found = true;
            INVARIANT(pdu contains referenced pdu instance)
            frame.pdus.push_back(std::make_pair(pdu,bit_pos));
            break;
		  }
		  if (!found) smc->warn_(-1,"Import of FIBEX data: no pdu found with id '"+pdu_inst_id+"' as referenced by frame with short name '"+frame.short_name+"'");
	  }
	  using elem_t = decltype(frame.pdus)::value_type;
	  std::sort(frame.pdus.begin(),frame.pdus.end(),[](elem_t const & a, elem_t const & b){return a.second < b.second;});
	  fibex_frames.push_back(frame);
  }

  for(auto & e : fibex_frames){
    auto frm = new ceps::ast::Struct("frame",nullptr,nullptr,nullptr);
    frm->children().push_back(new ceps::ast::Struct("id",new ceps::ast::Identifier(e.short_name,nullptr,nullptr,nullptr),nullptr,nullptr));

    ceps::ast::Struct_ptr data = new ceps::ast::Struct("data",nullptr,nullptr,nullptr);frm->children().push_back(data);
    ceps::ast::Struct_ptr payload;
    data->children().push_back(payload = new ceps::ast::Struct("payload",nullptr,nullptr,nullptr));
    ceps::ast::Struct_ptr in;
    payload->children().push_back(in = new ceps::ast::Struct("in",nullptr,nullptr,nullptr));
    ceps::ast::Struct_ptr out;
    payload->children().push_back(out = new ceps::ast::Struct("out",nullptr,nullptr,nullptr));
    for(auto & pdu_inst:e.pdus){
        size_t bit_offs = pdu_inst.second;
        for(auto & pdu : pdu_inst.first.sig_instances){
         bit_offs += pdu.bitposition;
         std::string bitwidth;
         if (sig_vec[pdu.sigidx].encoding.bit_length == "1")
        	 bitwidth ="bit";
         else
          bitwidth = cod2internal_cod[sig_vec[pdu.sigidx].encoding.base_data_type]+sig_vec[pdu.sigidx].encoding.bit_length;

         out->children().push_back(
        		 new ceps::ast::Struct(bitwidth,
        				               new ceps::ast::Symbol( sig_vec[pdu.sigidx].name,"Systemstate" ,nullptr,nullptr,nullptr)
                )
         );
         in->children().push_back(
        		 new ceps::ast::Struct(bitwidth,
        		  new ceps::ast::Symbol( sig_vec[pdu.sigidx].name ,"Systemstate",nullptr,nullptr,nullptr))
         );
       }
    }
    r.nodes().push_back(frm);
  }

  auto channels = ns["fx:ELEMENTS"]["fx:CHANNELS"][ceps::ast::all{"fx:CHANNEL"}];
  for(auto e : channels){
   auto channel = channels["fx:CHANNEL"];
   auto channel_id = channel["ho:SHORT-NAME"].as_str();
   if (channel_id.length() == 0) smc->fatal_(-1,"Import of FIBEX data: channel "+channel["@ID"].as_str() +" has no short name");
   auto sender = new ceps::ast::Struct("sender",new ceps::ast::Struct("id",new ceps::ast::Identifier(channel_id)));
   ceps::ast::Struct_ptr canbus = nullptr;
   auto transport = new ceps::ast::Struct("transport",canbus = new ceps::ast::Struct("canbus"));
   sender->children().push_back(transport);
   r.nodes().push_back(sender);

   auto frame_triggerings = channel["fx:FRAME-TRIGGERINGS"][ceps::ast::all{"fx:FRAME-TRIGGERING"}];
   if (frame_triggerings.size()){
    auto can_id_mapping = new ceps::ast::Struct("can_id_mapping");
    canbus->children().push_back(can_id_mapping);
    for(auto e : frame_triggerings){
    	auto ftrig = e["fx:FRAME-TRIGGERING"];
    	auto frame_id = std::stoi(ftrig["fx:IDENTIFIER"]["fx:IDENTIFIER-VALUE"].as_str());
    	std::string frame_name = "";
    	std::string frame_ref = ftrig["fx:FRAME-REF"]["@ID-REF"];
    	auto it = frm_id_to_frame_name.find(frame_ref);
    	if (it == frm_id_to_frame_name.end()) smc->fatal_(-1,"Import of FIBEX data: channel "+channel["@ID"].as_str() +" frame triggering: frame id '"+frame_ref+"' doesn't exist" );
    	frame_name = it->second;
    	can_id_mapping->children().push_back(new ceps::ast::Identifier(frame_name,nullptr,nullptr,nullptr));
    	can_id_mapping->children().push_back(new ceps::ast::Int(frame_id,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr));
    }
   }
  }//for

  //write encodings
  ceps::ast::Struct_ptr encoding = new ceps::ast::Struct("encoding",nullptr,nullptr,nullptr);

  for (auto sig : sig_vec){
   using namespace ceps::ast;
   auto out_scale = 1.0 / sig.encoding.internal_to_phs_scale;
   auto out_offs = -1.0*sig.encoding.internal_to_phs_offset*out_scale;
   auto out_encoding = new Binary_operator('=',
		                                   new Symbol("out",sig.encoding.ceps_encoding_type),
										   new Binary_operator('+',
												   new Binary_operator('*',new Double(out_scale, ceps::ast::all_zero_unit()), new Symbol(sig.name,"Systemstate")  ),
												   new Double(out_offs, ceps::ast::all_zero_unit())
                                           )
   );
   auto in_encoding = new Binary_operator('=',
		                                   new Symbol(sig.name,"Systemstate"),
										   new Binary_operator('+',
												   new Binary_operator('*',new Double(sig.encoding.internal_to_phs_scale, ceps::ast::all_zero_unit()), new Symbol("in",sig.encoding.ceps_encoding_type)  ),
												   new Double(sig.encoding.internal_to_phs_offset, ceps::ast::all_zero_unit())
                                           )
   );
   encoding->children().push_back(out_encoding);
   encoding->children().push_back(in_encoding);
  }

  r.nodes().push_back(encoding);

  //write constraints
  ceps::ast::Struct_ptr constraints = new ceps::ast::Struct("constraints",nullptr,nullptr,nullptr);

  for (auto sig : sig_vec){
   using namespace ceps::ast;
   auto a = new Binary_operator('<',
    									   new Symbol(sig.name,"Systemstate"),
 										   new Double(sig.encoding.max, ceps::ast::all_zero_unit())
    );
   auto b = new Binary_operator('>',
      									   new Symbol(sig.name,"Systemstate"),
   										   new Double(sig.encoding.min, ceps::ast::all_zero_unit())
    );
   constraints->children().push_back(a);
   constraints->children().push_back(b);
  }

  r.nodes().push_back(constraints);

  return r;
}
