//
//  PoolManager.cpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/17.
//  Copyright © 2019 ssyram. All rights reserved.
//

#include "PoolManager.hpp"
using std::function;

#include <forward_list>
#include <thread>
#include "semaphore.hpp"
using std::thread;
using std::ref;
using std::forward_list;
using std::mutex;
using std::unique_lock;
using std::lock_guard;
using std::condition_variable;

namespace utils {
    
    class impl_pool_manager {
        const size_t size;
        
        mutex el_mtx;
        forward_list<uint64_t> empty_list;
        
        function<void ()> *to_run_list;
        semaphore *smphr_list;
        semaphore *ret_list; // to wait for all of them to come back
        bool *stop_list;
        bool &stop_signal;

		std::optional<except::TranslateException> except;
        
        static void
        thread_function(
                        function<void ()> &to_run,              // the function to run
                        semaphore &smphr,                       // activation semaphore
                        semaphore &ret,                         // to signal when finish
                        bool &to_stop,                          // whether to stop this
                        bool &stop_signal,                      // the signal to stop all
                        mutex &el_mtx,                          // the mutex for empty_list
                        forward_list<uint64_t> &empty_list,     // the list to put this
						std::optional<except::TranslateException> &except,
                        size_t id
                        )
        {
            smphr.wait();
            while (!to_stop) {
				try {
					to_run();
				}
				catch (const except::TranslateException &e) {
					except = e;
					stop_signal = true;
				}
                
                el_mtx.lock();
                empty_list.push_front(id);
                el_mtx.unlock();
                if (to_stop || stop_signal) break;
                smphr.wait();
            }
            ret.signal();
        }
    public:
        impl_pool_manager(size_t pool_size, bool &stop_signal): stop_signal(stop_signal), size(pool_size) {
            to_run_list = new function<void ()>[pool_size];
            smphr_list = new semaphore[pool_size];
            ret_list = new semaphore[pool_size];
            stop_list = new bool[pool_size]();
            for (size_t i = 0; i < pool_size; ++i) {
                empty_list.push_front(i + 1);
                
                // start thread
                thread tr(&thread_function,
                          ref(to_run_list[i]),
                          ref(smphr_list[i]),
                          ref(ret_list[i]),
                          ref(stop_list[i]),
                          ref(this->stop_signal),
                          ref(el_mtx),
                          ref(empty_list),
						  ref(except),
                          i + 1);
                tr.detach();
            }
        }
        ~impl_pool_manager() {
			for (size_t i = 0; i < size; ++i) {
				stop_list[i] = true;
				smphr_list[i].signal();
			}
			for (size_t i = 0; i < size; ++i)
				ret_list[i].wait();
            
            delete [] to_run_list;
            delete [] smphr_list;
            delete [] stop_list;
        }
		std::optional<except::TranslateException> get_only_except() {
			return except;
		}
        void execute(function<void ()> to_run) {
            el_mtx.lock();
            if (empty_list.empty()) {
                el_mtx.unlock();
                to_run();
                return;
            }
            uint64_t k = empty_list.front() - 1;
            empty_list.pop_front();
            el_mtx.unlock();
            to_run_list[k] = to_run;
            smphr_list[k].signal();
        }
        bool has_empty() {
            return false;
        }
    };
    
    
    
    
    
    bool PoolManager::has_empty() {
        return impl->has_empty();
    }
    
    void PoolManager::execute(std::function<void ()> to_run) {
        impl->execute(to_run);
    }

	std::optional<except::TranslateException> PoolManager::get_only_except()
	{
		return impl->get_only_except();
	}
    
    PoolManager::PoolManager(size_t pool_size, bool &stop_signal) {
        impl = new impl_pool_manager(pool_size, stop_signal);
    }
    
    PoolManager::~PoolManager() {
        delete impl;
    }
    
    
}

