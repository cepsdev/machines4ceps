#ifndef INC_SM4CEPS_EVENTS_INC
#define INC_SM4CEPS_EVENTS_INC

#include <cstdint>
#include <iostream>
#include <memory>
#include <cstring>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace sm4ceps{



	class Ringbufferexception : public std::runtime_error
	{
	public:
		Ringbufferexception(const std::string & what) : std::runtime_error(what) {}
	};

	template<typename T> void call_destructor(T*){}


	template<typename R, size_t default_size = 1024> class Ringbuffer
	{
		char * data_ = nullptr;
		size_t capacity_ = 0;
		bool data_owner_ = false;
		size_t start = 0;
		size_t end = 0;
		size_t n_elems = 0;


		std::size_t append_record(R const & rec, size_t size)
		{
			auto t =  end;
			new (data_ + end) R{rec};
			end+=size;
			if (end >= capacity_) end=0;
			return t;
		}

		void cleanup()
		{
			if (!empty()){
				for(size_t i = 0; i < n_elems;++i){
					pop();
				}

			}
			if (data_owner_) delete[] data_;
		}

		std::int64_t next(std::int64_t ofs) const
		{
			if (ofs == -1) return -1;

			if (empty()) return -1;
			std::int64_t size = sizeof(R);
			ofs += size;
			if (ofs >= static_cast<int64_t>(capacity_)) ofs -= capacity_;
			if (ofs == start) return -1;
			if (ofs - 1 == end) return -1;
			return ofs;
		}

		R& get(std::int64_t ofs) const
		{
			return *((R*)(data_ + ofs));
		}

	public:
		Ringbuffer(char * data, size_t capacity, bool initialize = false)
			: data_(data),  capacity_(capacity)
		{
			init(data,  capacity, initialize);
		}

		Ringbuffer():Ringbuffer(default_size * sizeof(R)) {

		}

		void init(char * data,  size_t capacity, bool initialize = false)
		{
		    data_ = data;
		    n_elems = 0;
		    start = end=0;
		    capacity_ = capacity;
		    data_owner_ = false;
			if (initialize)
				clear();
		}

		explicit Ringbuffer(size_t capacity)
		{
			if (capacity < sizeof(R)) throw Ringbufferexception("Invalid Argument: Capacity too low.");
			data_ = new char[capacity_ = capacity];
			clear();
		}

		~Ringbuffer()
		{
			cleanup();
		}


		bool empty() const
		{

			return n_elems==0;
		}

		size_t count() const {return n_elems;}
		char const * data() const { return data_; }

		void clear()
		{
			start = 0;
			end = 0;
		}

		size_t available_space() const
		{
			return capacity_ - n_elems*sizeof(R);
		}

		size_t next_free_ofs() const {return end;}

		size_t capacity() const { return capacity_; }
		size_t allocated_space() const { return capacity() - available_space(); }

		size_t push(R const & rec) {return append(rec);}
		size_t append(R const & rec)
		{

			auto rec_size = sizeof(R);
			if (rec_size > capacity_) throw Ringbufferexception("insert: Record too large.");
			auto free_space = available_space();
			//std::cout << "free_space=" << free_space <<" end=" << end<< std::endl;
			n_elems+=1;
			if (rec_size <= free_space)
			{
				//std::cout << "******"<< rec << std::endl;
				return append_record(rec, rec_size);
			}
			else // we have to free some space
			{
				//INVARIANT: rec_size <= capacity_
				pop();
				// Loop terminates because of invariant
				//INVARIANT: rec_size <= available_space(rh)
				return append_record(rec, rec_size);
			}

		}

		size_t size() const {return n_elems;}

		R& get_at_index(size_t i) {auto ofs =(start+i*sizeof(R)) % capacity_; return get(ofs);}
		R& get_at_ofs(size_t i) {return get(i);}
		bool valid_ofs(size_t i) {return i < capacity_;}
		size_t find(R const & r, int offset = 0){
			if(empty()) return capacity_;
			auto s = start;
			if (offset != 0) {s+=offset;if(s >= capacity_) s= 0;}
			size_t i = 0;
                        for(auto ofs = s; i != n_elems; ofs+=sizeof(R),++i){
				if (ofs > capacity_) ofs= 0;
				if (get(ofs) == r) return ofs;
			}
			return capacity_;
		}


		bool pop()
		{
			if (empty()) return false;
			auto size = sizeof(R);
			call_destructor((R*)(data_+start));
			start += size;
			if (start >= capacity_ ) start = 0;
			n_elems-=1;
			return true;
		}

		void swap_to_front_and_pop(size_t ofs){
			if (ofs >= capacity_ || empty()) return;
			if (ofs == start) {pop(); return;};
			R temp = front();
			front() = get(ofs);
			get(ofs) = temp;
			pop();
		}

		R& front() {
			if (empty()) throw Ringbufferexception("Ringbuffer empty.");
			return get(start);
		}

		size_t get_start() {return start;}
	};


	template<typename T> struct Eventqueue_traits{using id_t =std::string;};
	template<typename T> bool is_unique_event(T const &) {return false;}
	template<typename T> typename Eventqueue_traits<T>::id_t id(T const &) {return typename Eventqueue_traits<T>::id_t{};}

	template<typename E,int defaultlength=128,int defaultlength_evinfo=1024> class Eventqueue{
		struct ev_info{
			typename Eventqueue_traits<E>::id_t id_;
			std::size_t stored_at_;
			bool operator == (ev_info const & rhs) {return id_ == rhs.id_;}
			ev_info(typename Eventqueue_traits<E>::id_t id, std::size_t stored_at) : id_(id), stored_at_(stored_at){}
		};
		Ringbuffer<E,defaultlength> data_;
		Ringbuffer<ev_info,defaultlength_evinfo> stored_unique_events;
	public:
		void push(E const & elem){
			if (is_unique_event(elem)){
				auto ofs = stored_unique_events.find(ev_info(id(elem),0));
				if(stored_unique_events.valid_ofs(ofs)){
					data_.get_at_ofs(stored_unique_events.get_at_ofs(ofs).stored_at_) = elem;
					return;
				}
				stored_unique_events.push(ev_info(id(elem),data_.push(elem)));
				return;
			}
			data_.push(elem);
		}
		E& front(){return data_.front();}
		void pop() {
			if (data_.empty()) return;
			if (stored_unique_events.empty()){data_.pop(); return;}
			if(!is_unique_event(front())){data_.pop(); return;}
			auto ofs = stored_unique_events.find(ev_info(id(front()),0));
			data_.pop();
			if (!stored_unique_events.valid_ofs(ofs)) return;
			stored_unique_events.swap_to_front_and_pop(ofs);
		}
		size_t count() const {return data_.count();}
		E& get_at_index(size_t i){
			return data_.get_at_index(i);
		}
		size_t number_of_unique_events() const {return stored_unique_events.count();}
		Ringbuffer<ev_info,defaultlength_evinfo>& unique_events() {return stored_unique_events;}

		bool empty() const {return count() == 0;}
	};
}


#endif
