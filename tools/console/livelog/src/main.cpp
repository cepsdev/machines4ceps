#include <iostream>
#include "core/include/livelog/livelogger.hpp"
#include "core/include/sockets/rdwrn.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"


livelog::Livelogger* log_entries_peer;


int main(int argc, char * argv[]){
 using namespace std::chrono_literals;

 log_entries_peer = new livelog::Livelogger(200,20000000);
 log_entries_peer->register_storage(sm4ceps::STORAGE_IDX2FQS,new livelog::Livelogger::Storage (4096*1000));

 sm4ceps::Livelogger_sink livelogger_sink(log_entries_peer);
 livelogger_sink.run();
 livelog::Livelogger::Storage::id_t next_id = -1;
 std::map<int,std::string> id2name;

 for(;;){
	 std::this_thread::sleep_for(.25s);log_entries_peer->flush();

	 {
		 auto r = log_entries_peer->find_storage_by_id(sm4ceps::STORAGE_IDX2FQS);
		 livelog::Livelogger::Storage* s =  std::get<0>(r->second);
		 std::mutex& m = *std::get<1>(r->second);
		 std::lock_guard<std::mutex> lk(m);
		 livelog::for_each(*s,
		 			[&](void* data,
		 				livelog::Livelogger::Storage::id_t id,
		 				livelog::Livelogger::Storage::what_t what,
		 				livelog::Livelogger::Storage::len_t len)
		 			{
		 				if (what == sm4ceps::STORAGE_WHAT_INT32_TO_STRING_MAP)
		 					sm4ceps::storage_read_entry(id2name,(char*)data);
		 			}
		 	);
	 }
	 {
		 if (id2name.size() == 0) continue;
		 std::lock_guard<std::mutex> lk1(log_entries_peer->mutex_trans2consumer());
		 for_each(log_entries_peer->trans_storage(), [&](char* data,livelog::Livelogger::Storage::id_t id,livelog::Livelogger::Storage::what_t what, livelog::Livelogger::Storage::len_t len ){
			 if (next_id > id) return true;
			 next_id = id+1;
			 if (what == sm4ceps::STORAGE_WHAT_CURRENT_STATES){
				 sm4ceps::extract_current_states_raw(data,len,[&](std::vector<int> states){
					 for(auto const & e : states) std::cout << id2name[e] << " ";
					 std::cout << std::endl;
				 });
			 }
		 });
	 }
 }
 //comm_stream_thread(0,"127.0.0.1","3000");
 return 0;
}
