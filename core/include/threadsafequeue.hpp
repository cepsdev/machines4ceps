#ifndef INC_THREAD_SAFE_QUEUE_HPP
#define INC_THREAD_SAFE_QUEUE_HPP

#include <unordered_set>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

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

        template<typename Rep,typename Period> bool wait_for_data_with_timeout(const std::chrono::duration<Rep,Period>& rel_time){
                std::unique_lock<std::mutex> lk(m_);
                return cv_.wait_for(lk,rel_time,[this]{return !data_.empty(); });
        }

        Q& data() {return data_;}
        std::mutex& data_mutex() const {return m_;}
};

#endif
