/*
Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "core/include/docgen/docgenerator_docwriter_html5.hpp"
#include <memory>
using namespace ceps::ast;

void ceps::docgen::Doc_writer_html5::start(std::ostream& os) {
   os << R"(<!DOCTYPE html>
<html lang=en>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="author" content="generated by ceps">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-EVSTQN3/azprG1Anm3QDgpJLIm9Nao0Yz1ztcQTwFspd3yD65VohhpuuCOmLASjC" crossorigin="anonymous">
<style></style>
</head>
<body>)";  

}

void ceps::docgen::Doc_writer_html5::end(std::ostream& os) {
  os << R"(




    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/js/bootstrap.bundle.min.js" integrity="sha384-MrcW6ZMFYlzcLA8Nl+NtUVF0sA7MsXsP1UyJoMp4YLEuNSfAP+JcXn/tWtIaxVXM" crossorigin="anonymous"></script>
    <script src="https://unpkg.com/@popperjs/core@2"></script>
    </body>
</html>
  )";
}

/*

The function ceps::docgen::Doc_writer_html5::handle_can_frame generates a table representation of structures of the following form:

frame{
 ControlAreaNetworkBus
 id{  heart_beat_can_frame }
 data{ header{
   cob_id{ uint11{1360} }
   rtr{
    bit{
     0
    }
   }
   data_length{
    uint4{
     __current_frame_size
    }
   }
  }
  payload{
   out{
    uint7{
     ControlPilotDutyCycle
    }
    bit{
     any
    }
    uint3{
     if ControlPilotState == "A":
      1
     elif ControlPilotState == "B":
      2
     elif ControlPilotState == "C":
      3
     elif ControlPilotState == "D":
      4
     elif ControlPilotState == "E":
      5
     elif ControlPilotState == "F":
      6
     else :
      0
    }
    uint2{
     if ProximityPinState == "Not_Connected":
      1
     elif ProximityPinState == "Connected":
      2
     elif ProximityPinState == "Connected_but_not_latched":
      3
     else :
      0
    }
    uint3{
     if ActualChargeProtocol == "DIN70121":
      0
     elif ActualChargeProtocol == "ISO15118":
      1
     elif ActualChargeProtocol == "Not_supported":
      2
     else :
      3
    }
    uint4{
     if ProgressState == "Standby":
      1
     elif ProgressState == "Authentication":
      2
     elif ProgressState == "ChargeParameter":
      3
     elif ProgressState == "CableCheck":
      4
     elif ProgressState == "Precharge":
      5
     elif ProgressState == "Charge":
      6
     elif ProgressState == "WeldingDetection":
      7
     elif ProgressState == "StopCharge":
      8
     elif ProgressState == "SessionStop":
      9
     elif ProgressState == "ShutOff":
      10
     elif ProgressState == "Error":
      11
     elif ProgressState == "Pause":
      12
     elif ProgressState == "LLC_AC":
      13
     else :
      0
    }
    uint2{
     if TCPStatus == "TCP_Connected":
      1
     elif TCPStatus == "TLS_Connected":
      2
     else :
      0
    }
    uint2{
     any
    }
    bit{
     EVAchargeSE_Version_EV
    }
    uint5{
     EVAchargeSE_Version_Major
    }
    uint5{
     EVAchargeSE_Version_Minor
    }
    uint5{
     EVAchargeSE_Version_Patch
    }
    bit{
     EVAchargeSE_Version_CustomLogic
    }
    uint3{
     any
    }
    uint3{
     if ProximityResistorState == "R0":
      1
     elif ProximityResistorState == "R100":
      2
     elif ProximityResistorState == "R220":
      3
     elif ProximityResistorState == "R680":
      4
     elif ProximityResistorState == "R1500":
      5
     elif ProximityResistorState == "RInf":
      6
     else :
      0
    }
   }
   in{
    uint7{
     ControlPilotDutyCycleCANIn
    }
    bit{
     any
    }
    uint3{
     ControlPilotStateCANIn
    }
    uint2{
     ProximityPinStateCANIn
    }
    uint3{
     ActualChargeProtocolCANIn
    }
    uint4{
     ProgressStateCANIn
    }
    uint2{
     TCPStatusCANIn
    }
    uint2{
     any
    }
    bit{
     EVAchargeSE_Version_EVCANIn
    }
    uint5{
     EVAchargeSE_Version_MajorCANIn
    }
    uint5{
     EVAchargeSE_Version_MinorCANIn
    }
    uint5{
     EVAchargeSE_Version_PatchCANIn
    }
    bit{
     EVAchargeSE_Version_CustomLogicCANIn
    }
    uint3{
     any
    }
    uint3{
     ProximityResistorStateCANIn
    }
   }
  }
 }
}*/


static void read_frame_and_build_bit_info_vectors(	
                          std::vector<std::tuple<int,int,std::vector<ceps::ast::Nodebase_ptr>,ceps::ast::Nodebase_ptr >>& frame_in,
 													std::vector<std::tuple<int,int,std::vector<ceps::ast::Nodebase_ptr>,ceps::ast::Nodebase_ptr >>& frame_out,
													int & bit_ctr_in, int & bit_ctr_out,ceps::ast::Struct& tplvl_struct)
{
  static std::map<std::string,std::tuple<int,bool> > n2bit_width{ 
    {"bit",{1,false}}, {"uint1",{1,false}}, {"uint2",{2,false}}, {"uint3",{3,false}}, {"uint4",{4,false}}, {"uint5",{5,false}}, 
    {"uint6",{6,false}}, {"uint7",{7,false}}, {"uint8",{8,false}}, 
    {"byte",{8,false}}, {"uint16",{16,false}}, {"uint32",{32,false}}, {"uint",{32,false}}, {"uint64",{64,false}},
    {"int1",{1,true}}, {"int2",{2,true}}, {"int3",{3,true}}, {"int4",{4,true}}, {"int5",{5,true}}, {"int6",{6,true}}, {"int7",{7,true}}, 
    {"int8",{8,true}}, {"int16",{16,true}}, {"int32",{32,true}}, {"int64",{64,true}}, {"int",{32,true}}  
  };

	bool payload_section_read{false};
	shallow_traverse_ex(tplvl_struct.children(), 
	
							[&](ceps::ast::Nodebase_ptr tree_node) -> bool { return !payload_section_read; },

							[&](ceps::ast::Nodebase_ptr tree_node) -> bool {
								if (is<Ast_node_kind::structdef>(tree_node) && name(as_struct_ref(tree_node)) == "payload" ){
                  struct Args{bool valid;int width; bool signedness; bool in;};
                  shallow_traverse_ex2(
                    as_struct_ref(tree_node).children(),
                    /*pushing*/
                    [&](ceps::ast::Nodebase_ptr tree_node,Args a ) -> bool {
                      if (a.valid){
                        auto& store = a.in ? frame_in : frame_out;
                        if (store.size()) {
                          std::get<2>(store.back()).push_back(tree_node);
                        }                    
                      }
                      return true;
                    },
                    /*checking*/
                    [&](ceps::ast::Nodebase_ptr tree_node,Args& a ) -> bool {
                      if (is<Ast_node_kind::structdef>(tree_node)){
                        auto& cur_struct { as_struct_ref(tree_node) };
                        if (name(cur_struct) == "in") {
                          a.in = true;
                          return true;
                        } else if (name(cur_struct) == "out"){
                          a.in = false;
                          return true;
                        }
                        auto it {n2bit_width.find(name(cur_struct))};
                        if (it == n2bit_width.end()) return !is_leaf(tree_node->kind());
                        a.valid = true; a.signedness = std::get<1>(it->second);a.width = std::get<0>(it->second);
                        auto& store = a.in ? frame_in : frame_out;
                        auto& bit_ctr = a.in ? bit_ctr_in : bit_ctr_out;
                        store.push_back({bit_ctr,a.width,{},nullptr});
                        bit_ctr += a.width;
                        return true;
                      }                                            
                      return is<Ast_node_kind::stmt>(tree_node) || is<Ast_node_kind::stmts>(tree_node); 
                    },
                    Args{false,0,false,true}
                  );
                  									
									payload_section_read = true;
									return false;
								} 
								return !is_leaf(tree_node->kind());}
							);
}

void ceps::docgen::Doc_writer_html5::handle_can_frame(  std::ostream& os,                          
                                        ceps::ast::Struct& tplvl_struct,
                                        std::vector<ceps::ast::Symbol*> toplevel_isolated_symbols)
{
	std::vector<std::tuple<int,int,std::vector<ceps::ast::Nodebase_ptr>, ceps::ast::Nodebase_ptr >> frame_in;
	std::vector<std::tuple<int,int,std::vector<ceps::ast::Nodebase_ptr>, ceps::ast::Nodebase_ptr >>frame_out;
	int bit_ctr_in = 0;
	int bit_ctr_out = 0;
	read_frame_and_build_bit_info_vectors(frame_in,frame_out,bit_ctr_in,bit_ctr_out,tplvl_struct);

  bool inside_id_struct{false};
  std::string can_id;

  shallow_traverse_ex(tplvl_struct.children(),	
							[&](ceps::ast::Nodebase_ptr tree_node) -> bool { 
                if (inside_id_struct && is<Ast_node_kind::identifier>(tree_node)){
                  can_id = name(as_id_ref(tree_node));
                  return false;
                }
                return true; },

							[&](ceps::ast::Nodebase_ptr tree_node) -> bool {
                if (can_id.size()) return false;
								if (is<Ast_node_kind::structdef>(tree_node) && name(as_struct_ref(tree_node)) == "id" ) inside_id_struct = true;
                return is<Ast_node_kind::stmt>(tree_node) || is<Ast_node_kind::stmts>(tree_node) || 
                       is<Ast_node_kind::structdef>(tree_node) || is<Ast_node_kind::scope>(tree_node);
              });

  auto bits_per_row = 8;


  os << R"(<div class="container">)";
  os << R"(<table class="table"> <thead> <tr> <th>)";
  os << can_id << "</th></tr></thead><tbody><tr><td>";

  os << R"(<table class="table table-bordered table-striped table-hover" style=\"border: 1px solid black;\">)";

  os << R"(
    <thead>
     <tr>)";

  for (auto i = 0; i < bits_per_row;++i) os << "<th> bit "<< i <<"</th>";

  os << R"(</tr>
    </thead>
  )";

  os << "<tbody>";

  int bit_pos = 0;
  int bits_left_in_row = bits_per_row;
  std::map<ceps::ast::Nodebase_ptr, std::vector<ceps::ast::Nodebase_ptr> > rep2expr;

  for (auto& e:frame_out){
    auto& nodes = std::get<2>(e);
    auto& representative = std::get<3>(e);
    shallow_traverse_ex(nodes,	
			[&](ceps::ast::Nodebase_ptr tree_node) -> bool {      
        if (representative) return false; 
        if (is<Ast_node_kind::symbol>(tree_node) && kind(as_symbol_ref(tree_node))=="Systemstate"){
          representative = tree_node;
          return false;
        }
        else if (is<Ast_node_kind::identifier>(tree_node) && name(as_id_ref(tree_node)) == "any"){
          representative = tree_node;
          return false;
        }
        return true; 
      },
			[&](ceps::ast::Nodebase_ptr tree_node) -> bool {
        return traversable_fragment(tree_node);
      });
      if (representative) rep2expr[representative] = nodes;
  }

  

  for (auto e:frame_out){
    auto representative = std::get<3>(e);

    auto w = std::get<1>(e);
    if (w > 0 && bits_left_in_row == bits_per_row) {os << "<tr>";}
    for(;w > 0;){
      if (bits_left_in_row == 0) {os << "</tr><tr>";bits_left_in_row=bits_per_row;}
      auto t = w > bits_left_in_row ? bits_left_in_row : w ;
      os << "<td style=\"border: 1px solid black;\" colspan=\""<< t <<"\">";
      if (representative) {
        if (is<Ast_node_kind::identifier>(representative) && "any" == name(as_id_ref(representative)))
         os << "-";
        else ceps::docgen::fmt_handle_node(os, representative, this,true);
      }
      os << "</td>\n";
      w -= t;
      bits_left_in_row -= t;
    }    
  }
  os << "</tr>";

  os << "</tbody>";

  os << "</table>";
  os << "</td></tr>";

  auto match_check_equality = [] (Nodebase_ptr compare_against, Nodebase_ptr p) -> std::tuple<bool,Nodebase_ptr,Nodebase_ptr,Nodebase_ptr >{
    std::tuple<bool,Nodebase_ptr,Nodebase_ptr,Nodebase_ptr > r {false,nullptr,nullptr,nullptr};
    if (is<Ast_node_kind::ifelse>(p) && as_ifelse_ref(p).children().size() && 
        is<Ast_node_kind::binary_operator>(as_ifelse_ref(p).children()[0]) && op_val(as_binop_ref(as_ifelse_ref(p).children()[0])) == "==" ){
          auto& comparison = as_binop_ref(as_ifelse_ref(p).children()[0]);
          std::stringstream s1; s1 << *compare_against;
          std::stringstream s2; s2 << *comparison.left();
          std::stringstream s3; s3 << *comparison.right();
          Nodebase_ptr rhs = nullptr;
          if (s1.str() == s2.str()) rhs = comparison.right();
          else if (s1.str() == s3.str()) rhs = comparison.left();
          else return r;
          return {true,rhs, as_ifelse_ref(p).children().size() > 1 ? as_ifelse_ref(p).children()[1] : nullptr, 
                            as_ifelse_ref(p).children().size() > 2 ? as_ifelse_ref(p).children()[2] : nullptr};
    }
    return r;
  };

  
  os << "</tbody></table>";

  os << "</div>";

  //Print further Infos

  if (rep2expr.size()){
    os << R"(<div class="container">)";
    for(auto e:rep2expr){
      if (is<Ast_node_kind::identifier>(e.first) && name(as_id_ref(e.first))== "any") continue;
      if (e.second.size() == 1 && is<Ast_node_kind::ifelse>( e.second[0] ) ){
        os << R"(<table class="table table-bordered"> <thead> <tr> <th colspan="2">)";
        ceps::docgen::fmt_handle_node(os, e.first, this,true);
        os << R"(</th></tr>)";
        os <<"<tr><th>Value</th><th>Encoding</th> </tr>";
        os << "<tbody>";
        auto cur_if = e.second[0];
        for(;cur_if;){
          auto r = match_check_equality(e.first,cur_if);
          if (!std::get<0>(r) || std::get<1>(r) == nullptr || std::get<2>(r) == nullptr) break;
          os << "<tr>";
          os << "<td>"; ceps::docgen::fmt_handle_node(os, std::get<1>(r), this,true); os << "</td>";
          os << "<td>"; ceps::docgen::fmt_handle_node(os, std::get<2>(r), this,true); os << "</td>";
          os << "</tr>";
          cur_if = std::get<3>(r);
        }
        os << "</tbody></table>";
      }
    }
    os << R"(</div>)";
  }

}

void ceps::docgen::Doc_writer_html5::handle_network_frame(  std::ostream& os,
                                        std::string network_frame,                                        
                                        ceps::ast::Struct& tplvl_struct,
                                        std::vector<ceps::ast::Symbol*> toplevel_isolated_symbols)
{
	if (network_frame == "ControlAreaNetworkBus" || network_frame == "CAN") handle_can_frame(os,tplvl_struct,toplevel_isolated_symbols);
}



bool ceps::docgen::Doc_writer_html5::handler_toplevel_struct( 
    std::ostream& os,
	std::vector<ceps::ast::Symbol*> toplevel_isolated_symbols, 
    ceps::ast::Struct& tplvl_struct) 
{
	for (auto e: toplevel_isolated_symbols) {
		if (ceps::ast::kind(*e) == "NetworkFrame") {
			handle_network_frame(os,ceps::ast::name(*e),tplvl_struct,toplevel_isolated_symbols);
			return true;
		}
	}
	return false;
}

void ceps::docgen::Doc_writer_html5::out(std::ostream& os, 
                             std::string s, 
							 MarginPrinter* mp) {

    auto& ctx = top(); 
    //os << "\033[0m"; //reset
	if(!ctx.ignore_indent) {
		if (mp != nullptr) mp->print_left_margin(os,ctx);
		else for(int i = 0; i < ctx.indent; ++ i) os << ctx.indent_str;
	}
	//os << "\033[0m"; //reset
	
	//if (ctx.text_foreground_color.length()) os << "\033[38;5;"<< theme->choose_color(ctx.text_foreground_color).as_ansi_8bit_str() << "m";
	
	if (ctx.underline);// os << "\033[4m";
	if (ctx.italic);// os << "\033[3m";
	if (ctx.bold);// os << "\033[1m";
	if (ctx.normal_intensity);// os << "\033[22m";
	if (ctx.faint_intensity);// os << "\033[2m";
	for(int i = 0; i < ctx.linebreaks_before;++i) os << ctx.eol; 

	os << ctx.prefix;
	os << s;

	
	if (ctx.info.size()){
		//os << "\033[0m"; //reset
		//os << "\033[2m";
		os << " (";
		for(size_t i = 0; i + 1 < ctx.info.size(); ++i)
		os << ctx.info[i] << ",";
		os << ctx.info[ctx.info.size()-1];
		os << ")";
		//os << "\033[0m"; //reset
		
		//if (ctx.text_foreground_color.size()) os << "\033[38;5;"<< theme->choose_color(ctx.text_foreground_color).as_ansi_8bit_str() << "m";
	}
	os << ctx.suffix;
	if (ctx.eol.length() && ctx.comment_stmt_stack->size() && !ctx.ignore_comment_stmt_stack){
		//os << "\033[0m";
		//os << "\033[2m";
		//os << "\033[3m";
		std::string eol_temp = ctx.eol;
		ctx.eol = "";
		ctx.suffix = "";
		print_comment(os,this);
		ctx.eol = eol_temp;
		ctx.comment_stmt_stack->clear();
	}
	//os << "\033[0m"; //reset
	os << ctx.eol; 

}