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


static void read_frame_and_build_bit_info_vectors(	std::vector<std::tuple<int,int,ceps::ast::Struct*>>& frame_in,
 													std::vector<std::tuple<int,int,ceps::ast::Struct*>> frame_out,
													int & bit_ctr_in, int & bit_ctr_out,ceps::ast::Struct& tplvl_struct)
{
	bool payload_section_read{false};
	shallow_traverse_ex(tplvl_struct.children(), 
	
							[&](ceps::ast::Nodebase_ptr tree_node) -> bool { return !payload_section_read; },

							[&](ceps::ast::Nodebase_ptr tree_node) -> bool {
								if (is<Ast_node_kind::structdef>(tree_node) && name(as_struct_ref(tree_node)) == "payload" ){

									
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
	std::vector<std::tuple<int,int,ceps::ast::Struct*>> frame_in;
	std::vector<std::tuple<int,int,ceps::ast::Struct*>> frame_out;
	int bit_ctr_in = 0;
	int bit_ctr_out = 0;
	read_frame_and_build_bit_info_vectors(frame_in,frame_out,bit_ctr_in,bit_ctr_out,tplvl_struct);


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
			continue;
		}
	}
	return true;
}

void ceps::docgen::Doc_writer_html5::out(std::ostream& os, 
                             std::string s, 
							 MarginPrinter* mp) {

    auto& ctx = top(); 
    os << "\033[0m"; //reset
	if(!ctx.ignore_indent) {
		if (mp != nullptr) mp->print_left_margin(os,ctx);
		else for(int i = 0; i < ctx.indent; ++ i) os << ctx.indent_str;
	}
	os << "\033[0m"; //reset
	
	if (ctx.text_foreground_color.length()) os << "\033[38;5;"<< theme->choose_color(ctx.text_foreground_color).as_ansi_8bit_str() << "m";
	
	if (ctx.underline) os << "\033[4m";
	if (ctx.italic) os << "\033[3m";
	if (ctx.bold) os << "\033[1m";
	if (ctx.normal_intensity) os << "\033[22m";
	if (ctx.faint_intensity) os << "\033[2m";
	for(int i = 0; i < ctx.linebreaks_before;++i) os << ctx.eol; 

	os << ctx.prefix;
	os << s;

	
	if (ctx.info.size()){
		os << "\033[0m"; //reset
		os << "\033[2m";
		os << " (";
		for(size_t i = 0; i + 1 < ctx.info.size(); ++i)
		os << ctx.info[i] << ",";
		os << ctx.info[ctx.info.size()-1];
		os << ")";
		os << "\033[0m"; //reset
		
		if (ctx.text_foreground_color.size()) os << "\033[38;5;"<< theme->choose_color(ctx.text_foreground_color).as_ansi_8bit_str() << "m";
	}
	os << ctx.suffix;
	if (ctx.eol.length() && ctx.comment_stmt_stack->size() && !ctx.ignore_comment_stmt_stack){
		os << "\033[0m";
		os << "\033[2m";
		os << "\033[3m";
		std::string eol_temp = ctx.eol;
		ctx.eol = "";
		ctx.suffix = "";
		print_comment(os,this);
		ctx.eol = eol_temp;
		ctx.comment_stmt_stack->clear();
	}
	os << "\033[0m"; //reset
	os << ctx.eol; 

}