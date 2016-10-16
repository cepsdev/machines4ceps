#ifndef SM_SIM_LOGGER_INC
#define SM_SIM_LOGGER_INC

#include <cstddef>
#include <cstring>
#include <tuple>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <endian.h>
#include <sys/timerfd.h>
#include <set>
#include <unordered_map>

namespace livelog{

class Livelogger{
public:
	struct Storage{
		using pos_t = std::size_t;
		using len_t = std::size_t;
		using id_t = ssize_t;
		using what_t = std::int32_t;

		char * data_ = nullptr;
		pos_t start_ = 0, end_ = 0, skip_ = 0, len_ = 0;
		id_t id_counter_= id_t{};

		struct chunk{
			len_t len_;
			id_t id_;
			what_t what_;
			len_t len() const {return len_;}
			len_t set_len(len_t v) {return len_ = v;}
			id_t id() const {return id_;}
			id_t set_id(id_t v) {return id_=v;}
			what_t what() const {return what_;}
			what_t set_what(what_t v) {return what_ = v;}
			char* data()  {return (char*)this+sizeof(chunk);}
		}__attribute__((packed));


		std::pair<bool,pos_t> push_back(void * mem, len_t len, what_t what, id_t id = -1);
		std::tuple<void *, len_t,chunk*> front() const;
		std::tuple<void *, len_t,chunk*> next(chunk* m) const;
		void pop();
		bool empty() const {return start_ == end_;}
		len_t available_space();
		Storage(std::size_t len);
		Storage() = default;
		Storage& operator =  (Storage& source) = default;
		Storage (Storage&& source);
		Storage& operator =  (Storage&& source);
		~Storage() {if (data_ == nullptr) delete[] data_;data_=nullptr;}
		len_t capacity() const {if(len_ == 0) return 0; return len_-1;}
		void clear() {start_ = 0; end_ = 0; skip_ = 0; len_ = 0;}
		template <typename F> bool write_ext(Storage::what_t what, F f, Storage::len_t n,std::mutex* mtx = nullptr, Storage::id_t id = -1){
			auto g = [&](){
				if(n+sizeof(Storage::chunk) > capacity()) return false;
				auto r = push_back(nullptr,n,what,id);
				if(r.first) f(data_+r.second+sizeof(Storage::chunk));
				else return false;
				return true;
			};
			if (mtx){
				std::lock_guard<std::mutex> lk(*mtx);
				return g();
			} else return g();
		}
	};

private:
	bool cis_moved_to_trans_ = false;
	void trans_thread_fn();
	void comm_stream_dispatcher_fn();
	void comm_stream_handler_fn(int id,struct sockaddr_storage,int sck);
	void spawn_trans_thread();
	bool handle_cmd(Storage::id_t&,std::uint32_t cmd,int sck);
	void fatal(int err, std::string msg);
	void warning(int err, std::string msg);

	struct Storagesstorage{
		static constexpr std::size_t default_size = 32;
		std::vector<Storage> storages_;
		std::size_t s_ = 0, e_= 0;
		Storagesstorage() = default;
		Storagesstorage(std::size_t len){storages_.resize(len);}
		bool empty() const {return s_ == e_;}
		std::size_t size() {if (e_ >= s_) return e_ - s_; else return storages_.size() - s_ + e_ -1;}
		void move_to_and_clear(Storage& s){
			if (storages_.size() == 0) storages_.resize(default_size);
			if (storages_.size() == 0) return;
			auto t = storages_[e_].data_;
			storages_[e_] = s;
			if (t == nullptr) t = new char[s.len_];
			s.data_ = t;s.start_ = s.skip_ = s.end_ = 0;
			e_ = (e_ + 1) % storages_.size();
			if (e_ == s_) s_ = (s_ + 1) % storages_.size();
			//std::cout << "** s_="<<s_ << "  e_="<< e_ << std::endl;
		}
	};
	Storage cis_storage_;  //the storage used by *this* thread to store logging information,
	Storage trans_storage_; //the storage used by *this* and *the other* thread to transfer the accumulated logging information
    // cis_storage is supposed to be much smaller than trans_storage_
	Storagesstorage storagesstorage_;
	Storagesstorage storagesstorage2_;

	mutable std::mutex mutex_cis2trans_;
	mutable std::mutex mutex_trans2consumer_;
	mutable std::atomic<bool> cis2trans_thread_idle_;
	std::condition_variable cv_trigger_cis2trans_;
	std::thread* trans_thread_ = nullptr;
	std::thread* comm_stream_dispatcher_thread_ = nullptr;
	bool shutdown_;
	bool write_through_ = true;
	std::string port_;
	std::unordered_map<int,std::tuple<Storage*,std::mutex*,Storage::id_t>> registered_storages_by_id_;
	using reg_storage_ref = std::unordered_map<int,std::tuple<Storage*,std::mutex*,Storage::id_t>>::iterator;
	bool reg_storage_ref_valid(reg_storage_ref it) {return it != registered_storages_by_id_.end();}
	bool send_storage(Storage::id_t& last_transmitted_id,Storage *,int sck);
	bool check_write_preconditions(Storage::len_t n);
	void if_space_low_do_cis_to_trans_transfer(Storage::len_t n);

public:
	static constexpr std::size_t default_cis_storage_size = 8192;
	static constexpr std::size_t default_trans_storage_size = default_cis_storage_size * 1024;

	Livelogger(std::size_t cis_storage_size,std::size_t trans_storage_size): cis_storage_{cis_storage_size},trans_storage_{trans_storage_size}
	{
		spawn_trans_thread();
	}

	Livelogger(): cis_storage_{default_cis_storage_size},trans_storage_{default_trans_storage_size}{
		spawn_trans_thread();
	}

	~Livelogger();

	Storage& cis_storage() {return cis_storage_;}
	Storage& trans_storage() {return trans_storage_;}
	void flush();
	bool write(Storage::what_t what, char* mem, Storage::len_t n, id_t id = -1);
	template <typename F> bool write_ext(Storage::what_t what, F f, Storage::len_t n, Storage::id_t id = -1){
		if(!check_write_preconditions(n)) return false;
		if_space_low_do_cis_to_trans_transfer(n);
		auto r = cis_storage().push_back(nullptr,n,what,id);
		if(r.first) f(cis_storage().data_+r.second+sizeof(Storage::chunk));
		else return false;
		return true;
	}
	bool reserve(Storage::what_t what, Storage::len_t n);
	static std::size_t overhead_per_memblock();
	bool& write_through(){return write_through_;}
	void publish(std::string port);
	bool register_storage(int i, Storage * s){registered_storages_by_id_[i]=std::make_tuple(s,new std::mutex,-1);}
	reg_storage_ref find_storage_by_id(int i){return registered_storages_by_id_.find(i);}
	std::mutex& mutex_trans2consumer() {return mutex_trans2consumer_;}
	template <typename F> bool foreach_registered_storage(F f){
     for(auto & e : registered_storages_by_id_ ){
    	 if(!f(e.first,std::get<0>(e.second),std::get<1>(e.second),std::get<2>(e.second))) return false;
     }
     return true;
	}
};


template<typename F> void for_each(Livelogger::Storage const & storage, F f){
	auto current = storage.front();
	while(std::get<2>(current)){
		auto& chunk = *std::get<2>(current);
		f(((char*)&chunk)+sizeof(chunk), chunk.id(),chunk.what(),chunk.len());
		current = storage.next(std::get<2>(current));
	}
}

template<typename F> bool for_each_ext(Livelogger::Storage const & storage, F f){
	auto current = storage.front();
	while(std::get<2>(current)){
		auto& chunk = *std::get<2>(current);
		if(!f(((char*)&chunk)+sizeof(chunk), &chunk)) return false;
		current = storage.next(std::get<2>(current));
	}
	return true;
}

constexpr int CMD_GET_NEW_LOG_ENTRIES = 1;
constexpr int START_USER_DEFINED_STORAGE_IDS = 0x100;

constexpr int LOG_TYPE_ID_INT = 1;
constexpr int LOG_TYPE_ID_INT32 = 2;
constexpr int LOG_TYPE_ID_UINT32 = 3;
constexpr int LOG_TYPE_ID_INT64 = 4;
constexpr int LOG_TYPE_ID_UINT64 = 5;


template<typename T> Livelogger::Storage::what_t what(T const & v);
template<> Livelogger::Storage::what_t what<int>(int const & v);
template<> Livelogger::Storage::what_t what<std::int32_t>(std::int32_t const & v);
template<> Livelogger::Storage::what_t what<std::uint32_t>(std::uint32_t const & v);
template<> Livelogger::Storage::what_t what<std::int64_t>(std::int64_t const  & v);
template<> Livelogger::Storage::what_t what<std::uint64_t>(std::uint64_t const & v);

template<typename T> void log(Livelogger& logger, T v){
	logger.write(livelog::what(v),(char*)&v,sizeof(v));
}

}
#endif
