#include "core/include/modelling/gensm.hpp"

namespace ceps{namespace ast{
ceps::ast::Nodebase_ptr box(std::tuple<std::string,std::string,ceps::ast::Nodebase_ptr > const & v);
}}

ceps::ast::Nodebase_ptr ceps::ast::box(std::tuple<std::string,std::string,ceps::ast::Nodebase_ptr > const & v){
 auto s = new
   strct{"Transition",
	     ident(std::get<0>(v)),
		 ident(std::get<1>(v))
		 };
 if (std::get<2>(v)) s->get_root()->children().push_back(std::get<2>(v));
 return s->get_root();
}


sm4ceps::modelling::gensm::sm::sm(std::string id_):id{id_}{

}

ceps::ast::Nodeset sm4ceps::modelling::gensm::sm::ns(){
	ceps::ast::Nodeset result;
	//auto s = new ceps::ast::Struct("Statemachine");
	using namespace ceps::ast;
	strct* s;
	if (header != nullptr)
	 s = new
     strct{"Statemachine",
		    strct{"id",ident{id}},
			*header,
			strct{"States",states},
			transitions
	 };
	else
	 s = new
	 strct{"Statemachine",
	       strct{"id",ident{id}},
		   strct{"States",states},
		   transitions
		};

	result.nodes().push_back(s->get_root());
	return result;
}

void sm4ceps::modelling::gensm::sm::add_state(std::string s){
	states.push_back(ceps::ast::ident(s));
}

void sm4ceps::modelling::gensm::sm::add_transition(std::string from, std::string to, ceps::ast::Nodebase_ptr g){
	transitions.push_back(std::make_tuple(from,to,g));
}
