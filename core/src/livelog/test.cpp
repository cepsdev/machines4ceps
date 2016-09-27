#include "core/include/livelog/livelogger.hpp"
#include "core/include/sm_livelog_storage_ids.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"

#include <iostream>
#include <cassert>
#include <chrono>

using namespace std;


template<typename T> void list_entries(std::ostream& os, livelog::Livelogger::Storage const & storage){

	if (storage.empty()) std::cout << "No entries\n";
	else livelog::for_each(storage,
			[&](void* data,livelog::Livelogger::Storage::id_t id,livelog::Livelogger::Storage::what_t what,livelog::Livelogger::Storage::len_t len)
			{
				os << *((T*)data) << " (what = " << what << " id = "<< id << ")" << std::endl ;
			}
	);
}

void list_entries_map_storage(std::ostream& os, livelog::Livelogger::Storage const & storage){

	if (storage.empty()) std::cout << "No entries\n";
	else livelog::for_each(storage,
			[&](void* data,livelog::Livelogger::Storage::id_t id,livelog::Livelogger::Storage::what_t what,livelog::Livelogger::Storage::len_t len)
			{
				os << "Map:" << std::endl;
				auto num_of_elems =*((std::size_t*)data);
				os << num_of_elems << std::endl;
			}
	);
}

int main(){
	using namespace livelog;
	using namespace std::chrono_literals;
/*	{
		Livelogger live_logger1(201,367);
		for(int i = 0; i != 100; ++i){
			live_logger1.cis_storage().push_back(&i,sizeof(i),i+1);
			std::cout << "\nEntries:\n";
			list_entries<int>(std::cout,live_logger1.cis_storage());
		}
	}
	{
		Livelogger live_logger1;
		for(int i = 0; i != 100; ++i){
			log(live_logger1, i);
			std::cout << "\nEntries:\n";
			list_entries<int>(std::cout,live_logger1.cis_storage());
		}
	} */

	/*{
		Livelogger live_logger1(404,40000);
		live_logger1.write_through() = true;
		for(int i = 0; i != 100; ++i){
			log(live_logger1, i);
			//std::cout << "\nEntries in cis storage:\n";
			//list_entries<int>(std::cout,live_logger1.cis_storage());
		}
		std::cout << "#Flushing buffers..." << std::endl;
		live_logger1.flush();std::this_thread::sleep_for(2s);

		std::cout << "Entries in trans storage:" << std::endl;
		list_entries<int>(std::cout,live_logger1.trans_storage());
	}*/
	{
		Livelogger live_logger1;
		sm4ceps::Livelogger_source livelogger_source(&live_logger1);
		executionloop_context_t ec;
		ec.current_states.resize(10);		
		//live_logger1.write_through() = false;
		Livelogger::Storage idx2fqs(4096);
		live_logger1.register_storage(sm4ceps::STORAGE_IDX2FQS,&idx2fqs);
		std::map<int,std::string> the_map = { {0,"a"}, {1,"b"}, {2,"c"}, {3,"d"}, {4,"e"} , {5,"f"},
				{6,"g"}, {7,"h"}, {8,"i"},  {9,"j"}
		};
		sm4ceps::storage_write(idx2fqs,the_map,std::get<1>(live_logger1.find_storage_by_id(sm4ceps::STORAGE_IDX2FQS)->second ));
		list_entries_map_storage(cout,idx2fqs);
		
		live_logger1.publish("3000");
		
		for(int i = 0; i != 100000; ++i){
			for(auto& e: ec.current_states) e = 0;
			ec.current_states[(i + 1) % 10] =  ec.current_states[i % 10] = 1;
			livelogger_source.log_current_states(ec);
			//log(live_logger1, i);//live_logger1.flush();
			//std::cout << "\nEntries in cis storage:\n";
			//list_entries<int>(std::cout,live_logger1.trans_storage());
			//std::this_thread::sleep_for(0.01s);
		}
		//list_entries<int>(std::cout,live_logger1.trans_storage());
		std::cout << "ok\n";
		std::this_thread::sleep_for(1000s);
	}
}
