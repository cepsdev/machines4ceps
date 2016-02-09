#include "core/include/events.hpp"
template<typename T, typename Q> class threadsafe_queue{
	Q data_;
	mutable std::mutex m_;
	std::condition_variable cv_;
public:
	void push(T const & elem){
		std::lock_guard<std::mutex> lk(m_);
		data_.push(elem);
		cv_.notify_one();
	}

	void wait_and_pop(T & elem){
		std::unique_lock<std::mutex> lk(m_);
		cv_.wait(lk, [this]{return !data_.empty(); });
		elem = data_.front();
		data_.pop();
	}

	void wait_for_data(){
		std::unique_lock<std::mutex> lk(m_);
		cv_.wait(lk, [this]{return !data_.empty(); });
	}

	Q& data() {return data_;}
	std::mutex& data_mutex() const {return m_;}
};


struct Event {int id=0;int payload=0;};
bool is_unique_event(Event const & ev) {return ev.id == 2 || ev.id == 7;}
int id(Event const & ev) {return ev.id;}


int main(int argc, char ** argv){
	std::cout << "evqueues_test_main" << std::endl;
	sm4ceps::Ringbuffer<int,2> q;
	std::cout << q.count() << " ";q.push(1);std::cout << "q.front()="<< q.front() << " q.count()=" << q.count() << "\n";

	std::cout << q.count() << " ";q.push(2);std::cout << "q.front()="<< q.front() << " q.count()=" << q.count() << "\n";
	std::cout << q.count() << " ";q.push(3);std::cout << "q.front()="<< q.front() << " q.count()=" << q.count() << "\n";
	std::cout << q.count() << " ";q.push(4);std::cout << "q.front()="<< q.front() << " q.count()=" << q.count() << "\n";
	std::cout << q.get_at_index(0) <<" " << q.get_at_index(1) << std::endl;
	std::cout << q.count() << " ";q.push(5);std::cout << "q.front()="<< q.front() << " q.count()=" << q.count() << "\n";
	q.pop();
	//std::cout << q.get_start() << std::endl;
	std::cout << q.front() << std::endl; q.pop();
	threadsafe_queue<Event, sm4ceps::Eventqueue<Event> > thdq;
	thdq.push(Event{1,1});
	thdq.push(Event{2,2});
	thdq.push(Event{3,3});
	thdq.push(Event{4,4});
	thdq.push(Event{5,5});
	thdq.push(Event{6,6});
	std::cout << "Content of event queue (1):\n";
	std::cout << "Number of unique events:" << thdq.data().number_of_unique_events() << std::endl;

	std::cout << thdq.data().count() << std::endl;
	for(int i = 0; i < thdq.data().count(); ++i){
		std::cout << "id=" << thdq.data().get_at_index(i).id << " payload=" << thdq.data().get_at_index(i).payload<< std::endl;
	}
	thdq.push(Event{2,3});
	std::cout << "Content of event queue (2):\n";
	std::cout << "Number of unique events:" << thdq.data().number_of_unique_events() << std::endl;

	for(int i = 0; i < thdq.data().count(); ++i){
		std::cout << "id=" << thdq.data().get_at_index(i).id << " payload=" << thdq.data().get_at_index(i).payload<< std::endl;
	}
	thdq.push(Event{2,4});
	std::cout << "Content of event queue (3):\n";
	std::cout << "Number of unique events:" << thdq.data().number_of_unique_events() << std::endl;

	for(int i = 0; i < thdq.data().count(); ++i){
		std::cout << "id=" << thdq.data().get_at_index(i).id << " payload=" << thdq.data().get_at_index(i).payload<< std::endl;
	}
	thdq.data().pop();thdq.data().pop();
	std::cout << "Content of event queue (4):\n";
	std::cout << "Number of unique events:" << thdq.data().number_of_unique_events() << std::endl;
	for(int i = 0; i < thdq.data().count(); ++i){
		std::cout << "id=" << thdq.data().get_at_index(i).id << " payload=" << thdq.data().get_at_index(i).payload<< std::endl;
	}
	return 0;
}
