#include "core/include/livelog/livelogger.hpp"
#include "core/include/sm_livelog_storage_ids.hpp"
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
		Livelogger live_logger1(404,100000*24);
		//live_logger1.write_through() = false;
		live_logger1.publish("3000");
		for(int i = 0; i != 100000; ++i){
			log(live_logger1, i);//live_logger1.flush();
			//std::cout << "\nEntries in cis storage:\n";
			//list_entries<int>(std::cout,live_logger1.trans_storage());
			//std::this_thread::sleep_for(0.01s);
		}
		//list_entries<int>(std::cout,live_logger1.trans_storage());
		std::cout << "ok\n";
		std::this_thread::sleep_for(1000s);
	}
}
