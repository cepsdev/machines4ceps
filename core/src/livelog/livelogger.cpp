#include "core/include/livelog/livelogger.hpp"
#include <iostream>

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif

template<> livelog::Livelogger::Storage::what_t livelog::what<std::int32_t>(std::int32_t const & ){return livelog::LOG_TYPE_ID_INT32;}
template<> livelog::Livelogger::Storage::what_t livelog::what<std::uint32_t>(std::uint32_t const & ){return livelog::LOG_TYPE_ID_UINT32;}
template<> livelog::Livelogger::Storage::what_t livelog::what<std::int64_t>(std::int64_t const  & ){return livelog::LOG_TYPE_ID_INT64;}
template<> livelog::Livelogger::Storage::what_t livelog::what<std::uint64_t>(std::uint64_t const & ){return livelog::LOG_TYPE_ID_UINT64;}


std::size_t livelog::Livelogger::overhead_per_memblock() {return sizeof(Storage::chunk); }


void livelog::Livelogger::clear_all(){
 {
  std::lock_guard<std::mutex> lk1(this->mutex_trans2consumer_);
  std::unique_lock<std::mutex> lk(this->mutex_cis2trans_);
  reseted_ = true;
  trans_storage_.clear();
  cis_storage_.clear();
  for(auto & e : storagesstorage_.storages_ )
      e.clear();
 }
}

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

             for_each2(s,[&](void* data,
                             livelog::Livelogger::Storage::id_t id,
                             livelog::Livelogger::Storage::what_t what,
                             livelog::Livelogger::Storage::len_t len,
                             std::uint64_t secs, std::uint64_t nsecs){
                 trans_storage_.push_back(data,len,what,id,secs,nsecs);
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

bool livelog::Livelogger::write(livelog::Livelogger::Storage::what_t what ,
                                char* mem ,
                                Storage::len_t n ,
                                id_t id ,
                                std::uint64_t secs ,
                                std::uint64_t nsecs){
	if(!check_write_preconditions(n)) return false;
	if_space_low_do_cis_to_trans_transfer(n);
    cis_storage().push_back(mem,n,what,id,secs,nsecs);
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
	--size_;
	auto p = reinterpret_cast<chunk*>(data_+start_);
	start_ += p->len_+sizeof(chunk);
	if(skip_ != 0 && start_ == skip_) { start_= 0; skip_=0;}
	if ( empty() ) {start_ = end_ = 0;}
}

std::pair<bool,livelog::Livelogger::Storage::pos_t> livelog::Livelogger::Storage::push_back(void * mem, len_t len, what_t what, id_t id,std::uint64_t secs, std::uint64_t nsecs)
{
 auto tot_len = len+sizeof(chunk);
 if (tot_len + 1 > len_) return std::make_pair(false,0);
 while (tot_len > available_space()) pop();

 auto write_chunk = [&](chunk* to){
   to->len_ = len;
   to->what_ = what;
   if (id >= 0) to->id_ = id;
   else to->id_ = id_counter_++;
   if(secs == 0){
    struct timespec t = {0};
    clock_gettime(CLOCK_REALTIME,&t);
    to->timestamp_secs = t.tv_sec;
    to->timestamp_nsecs = t.tv_nsec;}
   else {
    to->timestamp_secs = secs;
    to->timestamp_nsecs = nsecs;
   }
 };

 if (end_ + tot_len <= len_){
	 write_chunk((chunk*)(data_+end_));
	 if(mem) std::memcpy(data_+end_+sizeof(Storage::chunk),mem,len);
	 auto r = std::make_pair(true,end_);
	 end_ = (end_ + tot_len) % len_;
	 ++size_;return r;
 } else {
	 skip_ = end_;
	 end_ = 0;
	 write_chunk((chunk*)(data_+end_));
	 if(mem) std::memcpy(data_+end_+sizeof(Storage::chunk),mem,len);
	 auto r = std::make_pair(true,end_);
	 end_= tot_len;
	 ++size_;return r;
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


void livelog::Livelogger::fatal(int , std::string msg){
 std::cout << errno << " " << msg << std::endl;
}
void livelog::Livelogger::warning(int , std::string ){

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


void livelog::Livelogger::comm_stream_handler_fn(int ,struct sockaddr_storage claddr,int sck)
{
	char host[1024] = {0};
	char service[1024] = {0};
	char addrstr[2048] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_storage);


	if (getnameinfo((struct sockaddr*)&claddr,addrlen,host,1024,service,1024,0) == 0)
		snprintf(addrstr,2048,"[host=%s, service=%s]",host, service);
	else
		snprintf(addrstr,2048,"[host=?,service=?]");
    //std::cout << "livelog::Livelogger::comm_stream_handler_fn: " <<  addrstr << std::endl;
	Storage::id_t last_transmitted_id = -1;
	for(;;)
	{
		std::uint32_t cmd;
		auto r = recv(sck,reinterpret_cast<char*>(&cmd),sizeof(cmd),0);if (r <= 0)break;
        cmd = ntohl(cmd);
		if (livelog::CMD_FLUSH_MAIN_LOG_STORAGE == cmd){
			flush();
			continue;
		}
		std::uint64_t t;
        r = recv(sck,reinterpret_cast<char*>(&t),sizeof(t),0);
        if (r <= 0)break;
		t = be64toh(t);last_transmitted_id = static_cast<Storage::id_t>(t);
		try
		{
            if(!handle_cmd(last_transmitted_id,cmd,sck)){
              Storage::len_t l= 0;
              if ( send(sck, (char*) &l,sizeof(l),0)  != sizeof(l)){std::cout << "*** write failed" << std::endl; break;}
            }
		} catch(...){}
	}
    close(sck);
}

bool livelog::Livelogger::send_storage(Storage::id_t& last_transmitted_id,livelog::Livelogger::Storage * st,int sck){
	Storage::id_t filter_id =  last_transmitted_id + 1;
	Storage::id_t last_transmitted_id_new = last_transmitted_id;

    if(!for_each_ext(*st,[&](void* data,livelog::Livelogger::Storage::chunk* ch){
    	if (filter_id >=0 && filter_id > ch->id()) return true;
    	Storage::len_t data_len = sizeof(livelog::Livelogger::Storage::chunk) + ch->len_;
    	if ( ::write(sck,&data_len,sizeof(data_len)) != sizeof(data_len)) return false;
	    if ( ::write(sck,ch,sizeof(livelog::Livelogger::Storage::chunk)) != sizeof(livelog::Livelogger::Storage::chunk)) return false;
	    if ( ::write(sck,data,ch->len_) != ch->len_) return false;
	    last_transmitted_id_new = ch->id();
        return true;
	    })) return false;
    Storage::len_t sentinel = 0;
    if ( ::write(sck,&sentinel,sizeof(sentinel)) != sizeof(sentinel)) return false;
    last_transmitted_id = last_transmitted_id_new;
    return true;
}

bool livelog::Livelogger::handle_cmd(Storage::id_t& last_transmitted_id,std::uint32_t cmd,int sck){
	//std::cout << "livelog::Livelogger::handle_cmd("<<last_transmitted_id<<","<<cmd<<");" << std::endl;
	reg_storage_ref it;Storage::id_t temp_id = -1;
	if (cmd == livelog::CMD_GET_NEW_LOG_ENTRIES){
		std::lock_guard<std::mutex> lk(mutex_trans2consumer_);
		return send_storage(last_transmitted_id,&trans_storage(),sck);
	}else if (reg_storage_ref_valid(it = find_storage_by_id(cmd))){
		auto storage = std::get<0>(it->second);
		if( std::get<1>(it->second)){
			std::lock_guard<std::mutex> lk2( *std::get<1>(it->second));
			return send_storage(/*std::get<2>(it->second)*/last_transmitted_id,storage,sck);
		}else return send_storage(/*std::get<2>(it->second)*/last_transmitted_id,storage,sck);
	}
	return false;
}




