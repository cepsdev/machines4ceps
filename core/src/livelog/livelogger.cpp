#include "core/include/livelog/livelogger.hpp"
#include <iostream>


template<> livelog::Livelogger::Storage::what_t livelog::what<std::int32_t>(std::int32_t const & v){return livelog::LOG_TYPE_ID_INT32;}
template<> livelog::Livelogger::Storage::what_t livelog::what<std::uint32_t>(std::uint32_t const & v){return livelog::LOG_TYPE_ID_UINT32;}
template<> livelog::Livelogger::Storage::what_t livelog::what<std::int64_t>(std::int64_t const  & v){return livelog::LOG_TYPE_ID_INT64;}
template<> livelog::Livelogger::Storage::what_t livelog::what<std::uint64_t>(std::uint64_t const & v){return livelog::LOG_TYPE_ID_UINT64;}


std::size_t livelog::Livelogger::overhead_per_memblock() {return sizeof(Storage::chunk); }


void livelog::Livelogger::trans_thread_fn(){

 for(;;){
	 {
		std::unique_lock<std::mutex> lk(this->mutex_cis2trans_);
		while(!cis_moved_to_trans_ && !shutdown_) cv_trigger_cis2trans_.wait(lk);

		cis_moved_to_trans_ = false;
		if (shutdown_) break;
	 }
	 {
		 std::lock_guard<std::mutex> lk1(this->mutex_trans2consumer_);

		 {
		  std::lock_guard<std::mutex> lk2(this->mutex_cis2trans_);
		  if (storagesstorage_.empty()) continue;
		  storagesstorage2_.s_ = storagesstorage_.s_;
		  storagesstorage2_.e_ = storagesstorage_.e_;
		  if(storagesstorage2_.storages_.size() != storagesstorage_.storages_.size())
			  storagesstorage2_.storages_.resize(storagesstorage_.storages_.size());

		  for(int i = storagesstorage_.s_; i!=storagesstorage_.e_;i=(i+1) %storagesstorage_.storages_.size() ){
			  auto& s = storagesstorage_.storages_[i];
			  auto t = storagesstorage2_.storages_[i].data_;
			  if (t == nullptr) t = new char[s.len_];
			  storagesstorage2_.storages_[i] = s;
			  s.data_ = t;s.clear();
		  }

		  storagesstorage_.s_ = storagesstorage_.e_ = 0;
		 }

		 for(int i = storagesstorage2_.s_; i!=storagesstorage2_.e_;i=(i+1) %storagesstorage2_.storages_.size() ){
			 auto& s = storagesstorage2_.storages_[i];

			 for_each(s,[&](void* data,livelog::Livelogger::Storage::id_t id,livelog::Livelogger::Storage::what_t what,livelog::Livelogger::Storage::len_t len){

				 trans_storage_.push_back(data,len,what,id);
			 });
		 }

	 }
 }
}

void livelog::Livelogger::flush(){
 if (cis_storage().empty()) return;
 std::lock_guard<std::mutex> lk(mutex_cis2trans_);
 storagesstorage_.move_to_and_clear(cis_storage());
 cis_moved_to_trans_ = true;
 cv_trigger_cis2trans_.notify_one();
}

bool livelog::Livelogger::check_write_preconditions(livelog::Livelogger::Storage::len_t n){
	if(n+sizeof(Storage::chunk) > cis_storage().capacity()) return false;
	return true;
}
void livelog::Livelogger::if_space_low_do_cis_to_trans_transfer(livelog::Livelogger::Storage::len_t n){
	if (cis_storage().available_space() < n+sizeof(Storage::chunk) && write_through_){
		std::lock_guard<std::mutex> lk(mutex_cis2trans_);
		storagesstorage_.move_to_and_clear(cis_storage());
		if(storagesstorage_.size() >= storagesstorage_.storages_.size() / 2){
		 cis_moved_to_trans_ = true;
		 cv_trigger_cis2trans_.notify_one();
		}
	}
}

bool livelog::Livelogger::write(livelog::Livelogger::Storage::what_t what, char* mem, Storage::len_t n){
	if(!check_write_preconditions(n)) return false;
	if_space_low_do_cis_to_trans_transfer(n);
	cis_storage().push_back(mem,n,what);
	return true;
}
bool livelog::Livelogger::reserve(livelog::Livelogger::Storage::what_t what, Storage::len_t n){
	if(!check_write_preconditions(n)) return false;
	if_space_low_do_cis_to_trans_transfer(n);
	cis_storage().push_back(nullptr,n,what);
	return true;
}

std::tuple<void *, livelog::Livelogger::Storage::len_t,livelog::Livelogger::Storage::chunk*>
livelog::Livelogger::Storage::next(livelog::Livelogger::Storage::chunk* m) const {
	std::size_t ofs = ((char*)m)-data_;
	std::size_t ofs2 = ofs + m->len_ + sizeof(chunk);
	if ( (skip_ != 0 && ofs2 == skip_) || ofs2 == m->len_) ofs2 = 0;
	if (ofs2 == end_) return std::make_tuple(nullptr,0,nullptr);
	return std::make_tuple( data_+ofs2+sizeof(chunk) ,((chunk*)(data_+ofs2))->len(), (chunk*)(data_+ofs2));
}

livelog::Livelogger::Storage::len_t livelog::Livelogger::Storage::available_space(){
	if(empty()) return len_ - 1;
	if(start_ < end_) return std::max(len_ - (end_+1), start_ == 0 ? 0 : start_ - 1);
	return start_ - end_ - 1;
}

livelog::Livelogger::Storage::Storage (std::size_t len){
	data_ = new char [len_=len];
	if (data_ == nullptr) throw std::runtime_error("bad_alloc");
}

livelog::Livelogger::Storage::Storage (livelog::Livelogger::Storage&& source){
	if (!source.data_) return;
	start_ = source.start_;
	end_ = source.end_;
	skip_ = source.skip_;
	data_ = source.data_;
	source.data_ = nullptr;
}

livelog::Livelogger::Storage& livelog::Livelogger::Storage::operator =  (livelog::Livelogger::Storage&& source){
	if (!source.data_) return *this;
	if (data_) delete[] data_;
	start_ = source.start_;
	end_ = source.end_;
	skip_ = source.skip_;
	data_ = source.data_;
	source.data_ = nullptr;
	return *this;
}


std::tuple<void *,  livelog::Livelogger::Storage::len_t,livelog::Livelogger::Storage::chunk*> livelog::Livelogger::Storage::front() const{
	if (empty()) return std::make_tuple(nullptr,0,nullptr);
	auto elem = reinterpret_cast<chunk*>(data_+start_);
	return std::make_tuple(data_+start_+sizeof(chunk),elem->len(),elem);
}

void livelog::Livelogger::Storage::pop(){
	if (empty()) return;
	auto p = reinterpret_cast<chunk*>(data_+start_);
	start_ += p->len_+sizeof(chunk);
	if(skip_ != 0 && start_ == skip_) { start_= 0; skip_=0;}
	if ( empty() ) {start_ = end_ = 0;}
}

std::pair<bool,livelog::Livelogger::Storage::pos_t> livelog::Livelogger::Storage::push_back(void * mem, len_t len, what_t what, id_t id)
{
 auto tot_len = len+sizeof(chunk);
 if (tot_len + 1 > len_) return std::make_pair(false,0);
 while (tot_len > available_space()){
	 /*std::cout << "*start="<<start_<< " end= "<<end_<<" len= "<< len_ << " skip= " << skip_ <<"\n";*/
	 pop();
	 /*std::cout << "*start="<<start_<< " end= "<<end_<<" len= "<< len_ << " skip= " << skip_ <<"\n";*/
 }

 if (end_ + tot_len <= len_){
	 ((chunk*)(data_+end_))->len_ = len;
	 ((chunk*)(data_+end_))->what_ = what;

	 if (id >= 0) ((chunk*)(data_+end_))->id_ = id;
	 else ((chunk*)(data_+end_))->id_ = id_counter_++;

	 if(mem) std::memcpy(data_+end_+sizeof(Storage::chunk),mem,len);
	 auto r = std::make_pair(true,end_);
	 end_ = (end_ + tot_len) % len_;
	 return r;
 } else {
	 skip_ = end_;
	 end_ = 0;
	 ((chunk*)(data_+end_))->len_ = len;
	 ((chunk*)(data_+end_))->what_ = what;

	 if (id >= 0) ((chunk*)(data_+end_))->id_ = id;
	 else ((chunk*)(data_+end_))->id_ = id_counter_++;

	 if(mem) std::memcpy(data_+end_+sizeof(Storage::chunk),mem,len);
	 auto r = std::make_pair(true,end_);
	 end_= tot_len;
	 return r;
 }
}

void livelog::Livelogger::spawn_trans_thread(){
	shutdown_=false;
	trans_thread_ = new std::thread{&livelog::Livelogger::trans_thread_fn,this};
}

livelog::Livelogger::~Livelogger(){
	if (trans_thread_){
		{
	     std::lock_guard<std::mutex> lk(mutex_cis2trans_);
		 shutdown_ = true;
		 cv_trigger_cis2trans_.notify_all();
		}
		trans_thread_->join();
		delete trans_thread_;
	}
}


void livelog::Livelogger::fatal(int err, std::string msg){
 std::cout << errno << " " << msg << std::endl;
}
void livelog::Livelogger::warning(int err, std::string msg){

}

// Communication

void livelog::Livelogger::publish(std::string port){
 if (comm_stream_dispatcher_thread_ != nullptr) return;
 comm_stream_dispatcher_thread_ = new std::thread(&Livelogger::comm_stream_dispatcher_fn,this);
 port_ = port;
}

void livelog::Livelogger::comm_stream_dispatcher_fn()
{
	struct addrinfo hints;
	struct addrinfo* result, * rp;
	int lfd;

	memset(&hints,0,sizeof(hints));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;


    if (getaddrinfo(nullptr,port_.c_str(),&hints,&result) != 0)
    	{fatal(errno,"getaddrinfo failed");return;}

    int optval=1;

	for(rp=result;rp;rp=rp->ai_next)
	{
		lfd = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
		if(lfd == -1) continue;
		if (setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(char*)&optval,sizeof(optval))) {fatal(errno,"setsockopt");return;}
		if (bind(lfd,rp->ai_addr,rp->ai_addrlen) == 0) break;
		close(lfd);
	}
	if (rp == nullptr) {fatal(-1,"comm_dispatcher_thread:Could not bind socket to any address.port="+port_);return;}

	if (listen(lfd,5)==-1){fatal(errno,"listen");return;}

	freeaddrinfo(result);

	for(;;)
	{
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		struct sockaddr_storage claddr;
		int cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);

		if (cfd == -1){
			continue;
		}
		new std::thread{&Livelogger::comm_stream_handler_fn,this,0,claddr,cfd};
	}
}


void livelog::Livelogger::comm_stream_handler_fn(int id,struct sockaddr_storage claddr,int sck)
{
	char host[1024] = {0};
	char service[1024] = {0};
	char addrstr[2048] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_storage);

	if (getnameinfo((struct sockaddr*)&claddr,addrlen,host,1024,service,1024,0) == 0)
		snprintf(addrstr,2048,"[host=%s, service=%s]",host, service);
	else
		snprintf(addrstr,2048,"[host=?,service=?]");
	//std::cout << addrstr << "\n";

	Storage::id_t last_transmitted_id = -1;
	for(;;)
	{
		std::uint32_t cmd;
		auto r = recv(sck,reinterpret_cast<char*>(&cmd),sizeof(cmd),0);
		if (r <= 0)
		{
			break;return;
		}
		cmd = ntohl(cmd);
		try
		{
			if(!handle_cmd(last_transmitted_id,cmd,sck)) break;
		} catch(...){}
	}
	close(sck);
}

bool livelog::Livelogger::send_storage(Storage::id_t& last_transmitted_id,livelog::Livelogger::Storage * st,int sck){
	Storage::id_t filter_id =  last_transmitted_id;
	Storage::id_t last_transmitted_id_new;

    if(!for_each_ext(*st,[&](void* data,livelog::Livelogger::Storage::chunk* ch){
    	if (filter_id >=0 && filter_id > ch->id()) return true;
	    if ( ::write(sck,ch,sizeof(livelog::Livelogger::Storage::chunk)) != sizeof(livelog::Livelogger::Storage::chunk)) return false;
	    if ( ::write(sck,data,ch->len_) != ch->len_) return false;
	    last_transmitted_id_new = ch->id();
	    })) return false;
    Storage::len_t sentinel = 0;
    if ( ::write(sck,&sentinel,sizeof(sentinel)) != sizeof(sentinel)) return false;
    last_transmitted_id = last_transmitted_id_new;
    return true;
}

bool livelog::Livelogger::handle_cmd(Storage::id_t& last_transmitted_id,std::uint32_t cmd,int sck){
	reg_storage_ref it;Storage::id_t temp_id = -1;
	if (cmd == livelog::CMD_GET_NEW_LOG_ENTRIES){
		std::lock_guard<std::mutex> lk(mutex_trans2consumer_);
		return send_storage(last_transmitted_id,&trans_storage(),sck);
	}else if (reg_storage_ref_valid(it = find_storage_by_id(cmd))){
		auto storage = std::get<0>(it->second);
		if( std::get<1>(it->second)){
			std::lock_guard<std::mutex> lk2( *std::get<1>(it->second));
			return send_storage(std::get<2>(it->second),storage,sck);
		}else return send_storage(std::get<2>(it->second),storage,sck);
	}
	return false;
}




