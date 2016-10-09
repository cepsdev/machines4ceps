#include "core/include/state_machine.hpp"


std::map<std::string,State_machine*> State_machine::statemachines;

int state_rep_t::id() {
	if (id_ == -1){
		if (is_sm_) id_ = smp_->idx_;
		else {
			for(auto it = smp_->states().begin(); it != smp_->states().end(); ++it) {
					 auto state = *it;
					 if (sid_ == state->id()) {id_ = state->idx_; break;}
			}
			if(id_==-1){
				for(auto subsm: smp_->children()){
					if(subsm->id() == sid_){id_=subsm->idx_;break;}
				}
			}
		}
	}
	return id_;
}


State_machine* state_rep_t::containing_sm() const {
	if (!is_sm_) return smp_;
	return smp_->parent_;
}

void State_machine::clone_from(State_machine* rhs,int & counter,std::string const & q_id, bool (* resolve_imports) (State_machine &, void*) , void * context)
 {

	for (auto m: State_machine::statemachines)
	{
		for(auto& t: m.second->transitions())
		{
			assert(t.from().id_.size() > 0 || t.from().unresolved());
			assert(t.to().id_.size() > 0 || t.to().unresolved());
		}
	}




	  events_ = rhs->events_;
	  actions_ = rhs->actions_;
	  for(auto& a : actions_) a.associated_sm_ = this;
	  this->is_thread_ = rhs->is_thread_;
	  this->join_ = rhs->join_;
	  this->join_state_ = rhs->join_state_;
	  this->contains_threads_ = rhs->contains_threads_;
	  assert(rhs->complete_);



	  std::map<State_machine*,State_machine*> cloned_sms;
	  std::map<std::string,State_machine*> id_to_sm;



	  for(auto p:states()) delete p;
	  states().clear();
	  for(auto it = rhs->states().begin();it != rhs->states().end();++it)
	  {
		  State const & s = *(*it);
		  State dest_s(s);

		  if (!dest_s.is_sm_) {dest_s.smp_ = this;}
		  else   map_state(dest_s,cloned_sms, id_to_sm,counter,q_id,resolve_imports,context);

		  states().insert(new State(dest_s));
	  }//for


	  for(auto & t_source : rhs->transitions_)
	  {
		  auto t_dest = t_source;

		  if (t_dest.from_.unresolved()){

		  } else if (t_dest.from_.is_sm_)
		  {
			  assert(t_dest.from_.smp_ != nullptr);

			  auto it = cloned_sms.find(t_dest.from_.smp_);
			  if (it != cloned_sms.end()) t_dest.from_.smp_ = it->second;
			  else
			  {
				  if (rhs->has_child(t_dest.from_.smp_)) {
					  auto temp = new State_machine(counter++,t_dest.from_.smp_->id_,this,depth_+1);
					  std::string new_q_id = q_id+"."+t_dest.from_.smp_->id_;
					  State_machine::statemachines[new_q_id] = temp;
					  temp->clone_from(t_dest.from_.smp(),counter,new_q_id,resolve_imports,context);
					  assert(t_dest.from_.smp()->complete_);
					  cloned_sms[t_dest.from_.smp()] = temp;
					  id_to_sm[t_dest.from_.smp()->id()] = temp;
					  children_.push_back(temp);
					  t_dest.from_.smp_ = temp;
				  }
			  }
		  } else t_dest.from_.smp_ = this;

		  if (t_dest.to_.unresolved()){

		  }
		  else if (t_dest.to_.is_sm_)
		  {
			assert(t_dest.to_.smp_ != nullptr);
		  	auto it = cloned_sms.find(t_dest.to_.smp_);
		  	if (it != cloned_sms.end()) t_dest.to_.smp_ = it->second;
		  	else
		  	{
		  	  if (rhs->has_child(t_dest.to_.smp_)) {
		  		  auto temp = new State_machine(counter++,t_dest.to_.smp_->id_,this,depth_+1);
		  		  std::string new_q_id = q_id+"."+t_dest.to_.smp_->id_;
		  		  State_machine::statemachines[new_q_id] = temp;
		  		  if (!t_dest.to_.smp()->complete_)
		  			  assert(resolve_imports(*t_dest.to_.smp(),context));

		  		  assert(t_dest.to_.smp()->complete_);
		  		  temp->clone_from(t_dest.to_.smp(),counter,new_q_id,resolve_imports,context);
		  		  cloned_sms[t_dest.to_.smp()] = temp;
		  		  id_to_sm[t_dest.to_.smp()->id()] = temp;
		  		  children_.push_back(temp);
		  		  t_dest.to_.smp_ = temp;
		  	  }
		  	 }
		   } else t_dest.to_.smp_ = this;

		  transitions_.push_back(t_dest);
	  }
	  for(auto child: rhs->children())
	  {
		  auto it = cloned_sms.find(child);
		  if (it != cloned_sms.end()) children().push_back(it->second);
		  else
		  {
			  auto temp = new State_machine(counter++,child->id_,this,depth_+1);
			  std::string new_q_id = q_id+"."+child->id_;
			  State_machine::statemachines[new_q_id] = temp;
	  		  if (!child->complete_)
	  			  assert(resolve_imports(*child,context));
			  temp->clone_from(child,counter,new_q_id,resolve_imports,context);
			  cloned_sms[child] = temp;
			  id_to_sm[child->id()] = temp;
			  children_.push_back(temp);
		  }
	  }

	  if (join_state_.unresolved()){

	  } else if (join_state_.is_sm_)
	  {
		 assert(join_state_.smp_ != nullptr);

		 auto it = cloned_sms.find(join_state_.smp_);
		 if (it != cloned_sms.end()) join_state_.smp_ = it->second;
		 else  {
			if (rhs->has_child(join_state_.smp_)) {
			 auto temp = new State_machine(counter++,join_state_.smp_->id_,this,depth_+1);
			 std::string new_q_id = q_id+"."+join_state_.smp_->id_;
			 State_machine::statemachines[new_q_id] = temp;
			 assert(join_state_.smp()->complete_);
			 temp->clone_from(join_state_.smp(),counter,new_q_id,resolve_imports,context);
			 cloned_sms[join_state_.smp()] = temp;
			 id_to_sm[join_state_.smp()->id()] = temp;
			 children_.push_back(temp);
			 join_state_.smp_ = temp;
			}
		 }
		} else join_state_.smp_ = this;




		for (auto m: State_machine::statemachines)
		{
			for(auto& t: m.second->transitions())
			{
				assert(t.from().id_.size() > 0 || t.from().unresolved());
				assert(t.to().id_.size() > 0 || t.to().unresolved());
			}
		}

 }


