#include "core/include/livelog/livelogger.hpp"
#include "core/include/sm_livelog_storage_ids.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"
#include "core/include/sockets/rdwrn.hpp"
#include <chrono>
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

constexpr short DATA_INT64  = 1;
constexpr short DATA_SZ     = 2;
constexpr short DATA_DOUBLE = 3;
constexpr short DATA_VEC_OF_VARIANTS = 4;

void sm4ceps::livelog_write(livelog::Livelogger& live_logger,executionloop_context_t::states_t const &  states){
 std::size_t len = 0;
 for(auto const & s : states) if (s != 0) len+=sizeof(std::uint32_t);
 live_logger.write_ext(sm4ceps::STORAGE_WHAT_CURRENT_STATES,[&](char * data){
	 std::uint32_t counter = 0;
	 for(auto const & s : states){
		 if (s != 0) {*((std::uint32_t*)data) = counter; data += sizeof(std::uint32_t);} ++counter;
	 }
  },len);
}

void sm4ceps::livelog_write(livelog::Livelogger& live_logger,std::pair<std::string,std::vector<sm4ceps_plugin_int::Variant>> const & event){
 std::size_t len = storage_write_event(nullptr,
                        std::get<0>(event),
						std::get<1>(event),false);
 live_logger.write_ext(sm4ceps::STORAGE_WHAT_EVENT,[&](char * data){
	 storage_write_event(data, std::get<0>(event), std::get<1>(event),true);
  },len);
}
void sm4ceps::livelog_write_console(livelog::Livelogger& live_logger,std::string const & s){
 std::size_t len = storage_write(nullptr, s,false);
 live_logger.write_ext(sm4ceps::STORAGE_WHAT_CONSOLE,[&](char * data){
	 storage_write(data, s,true);
  },len);
}

void sm4ceps::livelog_write_info(livelog::Livelogger& live_logger,std::string const & s){
 std::size_t len = storage_write(nullptr, s,false);
 live_logger.write_ext(sm4ceps::STORAGE_WHAT_INFO,[&](char * data){
	 storage_write(data, s,true);
  },len);
}

void sm4ceps::storage_write(livelog::Livelogger::Storage& storage,std::map<int,std::string> i2s, std::mutex* mtx){
	std::size_t len = sizeof(std::size_t);
	for(auto const & s : i2s){
		len += sizeof(std::int32_t);
		len += s.second.length()+1;
	}
	for(auto const & s : i2s)  len+=sizeof(std::int32_t)+s.second.length()+sizeof(std::int32_t);
	 storage.write_ext(sm4ceps::STORAGE_WHAT_INT32_TO_STRING_MAP,[&](char * data){
		 std::uint32_t counter = 0;
		 *((std::size_t*) data) = i2s.size();
		 data+=sizeof(std::size_t);
		 for(auto const & s : i2s){
			 *((std::int32_t*)data) = s.first;
			 data += sizeof(std::int32_t);
			 memcpy(data,s.second.c_str(),s.second.length());
			 *(data+s.second.length()) = 0;
			 data+=s.second.length()+1;
		 }
	  },len,mtx);
}
bool sm4ceps::storage_read_entry(std::map<int,std::string>& i2s, char * data){
	if (data == nullptr) return false;
	auto num_elems = *((std::size_t*)data);
	if (num_elems == 0) return true;
	data+=sizeof(std::size_t);
	for(std::size_t i = 0 ; i != num_elems;++i){
		auto a = *((std::int32_t*)data);data+=sizeof(std::int32_t);
		i2s[a] = std::string(data);data+=strlen(data)+1;
	}
}
//

std::size_t sm4ceps::storage_write(char * data,
 		                   std::string const & s,
						   bool write){
 std::size_t r = 0;
 if(write) *((short*)(data+r)) = DATA_SZ;
 r += sizeof(short);
 if (write) *((std::uint32_t*)(data+r)) = s.length()+1;
 r += sizeof(std::uint32_t);
 if(write) memcpy(data+r,s.c_str(),s.length()+1);
 r += s.length()+1;
 return r;
}

std::size_t sm4ceps::storage_write(char * data,
 		                   std::int64_t j,
 						   bool write){
 std::size_t r = 0;
 if(write) *((short*)(data+r)) = DATA_INT64;
 r += sizeof(short);
 if(write) *((std::int64_t*)(data+r)) = j;
 r += sizeof(std::int64_t);
 return r;
}

std::size_t sm4ceps::storage_write(char * data,
  		                double v,
  						bool write){
 std::size_t r = 0;
 if(write) *((short*)(data+r)) = DATA_DOUBLE;
 r += sizeof(short);
 if(write) *((double*)(data+r)) = v;
 r += sizeof(double);
 return r;
}

std::size_t sm4ceps::storage_write(char * data,
		                std::vector<sm4ceps_plugin_int::Variant> const & v,
  						bool write){
 std::size_t r = 0;
 if(write) *((short*)(data+r)) = DATA_VEC_OF_VARIANTS;
 r += sizeof(short);
 if(write) *((std::uint32_t*)(data+r)) = v.size();
 r += sizeof(std::uint32_t);
 for(auto const & e : v){
	 if (e.what_ == sm4ceps_plugin_int::Variant::Int)
		 r+=storage_write(data+r,(std::int64_t)e.iv_,write);
	 else if (e.what_ == sm4ceps_plugin_int::Variant::Double)
		 r+=storage_write(data+r,e.dv_,write);
	 else if (e.what_ == sm4ceps_plugin_int::Variant::String)
		 r+=storage_write(data+r,e.sv_,write);
 }
 return r;
}


//Event serialization/deserialization

void sm4ceps::storage_write_event(livelog::Livelogger::Storage& storage,
		                   std::string ev_name,
						   std::vector<sm4ceps_plugin_int::Variant>const & params,
						   std::mutex* mtx){
 std::size_t len = storage_write_event(nullptr,ev_name, params,false);
 storage.write_ext(sm4ceps::STORAGE_WHAT_EVENT,[&](char * data){
	 storage_write_event(data,ev_name, params,true);
 },len,mtx);
}

std::size_t sm4ceps::storage_write_event(char * data,
		                   std::string ev_name,
						   std::vector<sm4ceps_plugin_int::Variant> const & params,
						   bool write){
 std::size_t r = 0;
 r += storage_write(data+r,ev_name,write);
 r += storage_write(data+r,params,write);
 return r;
}

//Reads

std::size_t sm4ceps::storage_read_variant(char * data,sm4ceps_plugin_int::Variant& v ){
  std::size_t r = 0;
  short what = *((short*)(data+r));
  r+=sizeof(short);
  if (what == DATA_INT64) {std::int64_t vv; r+=storage_read(data+r,vv); v.iv_ = (int)vv;v.what_= sm4ceps_plugin_int::Variant::Int;}
  else if (what == DATA_DOUBLE) {r+=storage_read(data+r,v.dv_);v.what_= sm4ceps_plugin_int::Variant::Double;}
  else if (what == DATA_SZ) {r+=storage_read(data+r,v.sv_);v.what_= sm4ceps_plugin_int::Variant::String;}


  return r;
}

std::size_t sm4ceps::storage_read(char * data, std::vector<sm4ceps_plugin_int::Variant> & v){
 std::size_t r = 0;
 std::size_t len = *(std::uint32_t*)(data+r);

 r+= sizeof (std::uint32_t);
 for(std::size_t i = 0; i!=len;++i){
     sm4ceps_plugin_int::Variant vv;
     r += storage_read_variant(data+r,vv);
     v.push_back(vv);
 }
 return r;
}

std::size_t sm4ceps::storage_read(char * data, std::string & s){
 std::size_t r = 0;
 std::size_t len = *(std::uint32_t*)(data+r);
 r+=sizeof(std::uint32_t);
 s = data + r;
 return r + s.length() + 1;
}

std::size_t sm4ceps::storage_read(char * data, std::int64_t& v){
 std::size_t r = 0;
 v = *((std::int64_t*)(data+r));
 return r+sizeof(std::int64_t);
}

std::size_t sm4ceps::storage_read(char * data, double & v){
 std::size_t r = 0;
 v = *((double*)(data+r));
 return r+sizeof(double);
}

bool sm4ceps::storage_read_event(std::string& ev_name,
		   	   	   	   	  std::vector<sm4ceps_plugin_int::Variant>& params,
						  char * data){
 std::size_t r = sizeof(short);
 r+=storage_read(data+r,ev_name);
 r+= sizeof(short); //jump over magic number
 r+=storage_read(data+r,params);
 return r != 0;
}

void sm4ceps::Livelogger_sink::comm_stream_fn(int id,
			     std::string ip,
			     std::string port){
	int cfd;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	bool conn_established = false;
	std::map<int,ssize_t> logid2last_read_id;
	std::uint32_t no_data_received = 0;
	for(;;)
	{
		rp = nullptr;result = nullptr;
		if (!conn_established)
		{
			for(;rp == nullptr;)
			{
			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_canonname = NULL;
			hints.ai_addr = NULL;
			hints.ai_next = NULL;
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
            //hints.ai_flags = AI_NUMERICSERV;
			if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0){
                std::this_thread::sleep_for(std::chrono::microseconds(1000));
                continue;
			}
			if (result == nullptr) {
                std::this_thread::sleep_for(std::chrono::microseconds(1000));
                continue;
			}
			 for (rp = result; rp != NULL; rp = rp->ai_next) {
			  cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			  if (cfd == -1) {  continue; }
			  if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
			  close(cfd);
			 }
			 if (result != nullptr) freeaddrinfo(result);
			 if (rp == nullptr) {
                 std::this_thread::sleep_for(std::chrono::microseconds(1000000));
                 continue;
			 }
			}
			conn_established = true;
            logid2last_read_id.clear();
            if (livelogger_) livelogger_->clear_all();
		}

		//Read main log
        if (no_data_received >= 3){
		 auto cmd = livelog::CMD_FLUSH_MAIN_LOG_STORAGE;cmd = htonl(cmd);
		 if ( ( write(cfd, (char*) &cmd,sizeof(cmd) )) != sizeof(cmd) )
		  {
			close(cfd);conn_established=false;continue;
		  }
		}

		livelog::Livelogger::Storage::id_t last_read_id = -1;
		if (logid2last_read_id.find(livelog::CMD_GET_NEW_LOG_ENTRIES) == logid2last_read_id.end()){
			logid2last_read_id[livelog::CMD_GET_NEW_LOG_ENTRIES] = last_read_id;
		} else last_read_id = logid2last_read_id[livelog::CMD_GET_NEW_LOG_ENTRIES];

        std::uint32_t cmd = livelog::CMD_GET_NEW_LOG_ENTRIES;
		cmd = htonl(cmd);
		std::uint64_t t = htobe64((std::uint64_t)last_read_id);
		std::size_t chunks_read = 0;

		if ( ( write(cfd, (char*) &cmd,sizeof(cmd) )) != sizeof(cmd) )
		{
			close(cfd);conn_established=false;continue;
		}
		if ( ( write(cfd, (char*) &t,sizeof(t) )) != sizeof(t) )
		{
			close(cfd);conn_established=false;continue;
		}

		livelog::Livelogger::Storage::len_t len = 0;
		char * buffer = nullptr;
		std::size_t buffer_size = 0;

		for(;;) {
		 if ( sizeof(len) != read(cfd,&len,sizeof(len)) ){
			break;
		 }
		 if (len == 0) //sentinel read
		  break;
		 if (buffer_size < len){
			 if (buffer != nullptr) delete[] buffer;
			 buffer = new char[buffer_size = len * 2];
		 }
		 int r=0;
		 if(len != (r=readn(cfd,buffer,len))) { auto e= errno;std::cout <<"error:" << e << " r=" << r << std::endl;break;}

		 if (livelogger_){
			 auto ch = (livelog::Livelogger::Storage::chunk*)buffer;
			 auto data = buffer + sizeof(livelog::Livelogger::Storage::chunk);

             livelogger_->write(ch->what(),data,ch->len(),ch->id(),ch->timestamp_secs, ch->timestamp_nsecs);
			 logid2last_read_id[livelog::CMD_GET_NEW_LOG_ENTRIES] = ch->id();
			 ++chunks_read;
		 }
		}//Finished looping through input data

		if (chunks_read == 0) ++no_data_received;
		else no_data_received = 0;
        //std::cout << "chunks_read="<< chunks_read << std::endl;
        if (livelogger_){
			if(!livelogger_->foreach_registered_storage(
			 [&](int idx,livelog::Livelogger::Storage * storage,std::mutex* pmutex, livelog::Livelogger::Storage::id_t next_id){
				cmd = htonl(idx);
				livelog::Livelogger::Storage::id_t last_read_id = -1;
				if (logid2last_read_id.find(idx) == logid2last_read_id.end()){
					logid2last_read_id[idx] = last_read_id;
				} else last_read_id = logid2last_read_id[idx];
				std::uint64_t t = htobe64((std::uint64_t)last_read_id);
				if ( ( write(cfd, (char*) &cmd,sizeof(cmd) )) != sizeof(cmd) )
				{
                    close(cfd);conn_established=false;return false;
				}
				if ( ( write(cfd, (char*) &t,sizeof(t) )) != sizeof(t) )
				{
                    close(cfd);conn_established=false;return false;
				}


				for(;;) {
				 if ( sizeof(len) != read(cfd,&len,sizeof(len)) ){
                  break;
				 }
                 if (len == 0) { //sentinel read
				  break;
                 }
				 if (buffer_size < len){
					 if (buffer != nullptr) delete[] buffer;
					 buffer = new char[buffer_size = len * 2];
				 }
				 int r=0;
				 if(len != (r=readn(cfd,buffer,len))) { auto e= errno;std::cout <<"error:" << e << " r=" << r << std::endl;break;}
				 auto ch = (livelog::Livelogger::Storage::chunk*)buffer;
				 auto data = buffer + sizeof(livelog::Livelogger::Storage::chunk);
				 storage->write_ext(ch->what(), [&](char* to){ memcpy(to,data,ch->len());}, ch->len(),pmutex, ch->id());
				 logid2last_read_id[idx] = ch->id();

				}
		 	 }
			))continue;
		}

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	if (conn_established)close(cfd);
}

void sm4ceps::Livelogger_sink::run_sync(){
	comm_stream_fn(-1,ip_,port_);
}

void sm4ceps::Livelogger_sink::run(){
	attached_thread_ = new std::thread{&sm4ceps::Livelogger_sink::comm_stream_fn,this,-1,ip_,port_};
}
