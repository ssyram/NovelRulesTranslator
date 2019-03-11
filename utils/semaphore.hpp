//
//  semaphore.hpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/18.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef semaphore_hpp
#define semaphore_hpp

#include <mutex>
#include <condition_variable>

namespace utils {
    using std::mutex;
    using std::unique_lock;
    using std::condition_variable;
    
    // my implementation
    //    class semaphore {
    //        mutex mtx;
    //        int size;
    //
    //        condition_variable cv;
    //    public:
    //        semaphore(int size = 1): size(size) { }
    //        void wait() {
    //            unique_lock<mutex> lk(mtx);
    //            --size;
    //            if (size < 0)
    //                cv.wait(lk);
    //        }
    //        void signal() {
    //            unique_lock<mutex> lk(mtx);
    //            ++size;
    //            cv.notify_one();
    //        }
    //    };
    
    // implementation found on google
    class semaphore {
        mutex mtx;
        int count;
        
        semaphore(const semaphore &) = delete;
        void operator=(const semaphore &) = delete;
        
        condition_variable cv;
    public:
        semaphore(int count = 0): count(count) { }
        void wait() {
            unique_lock<mutex> lk(mtx);
            cv.wait(lk, [=] { return count > 0; });
            --count;
        }
        void signal() {
            unique_lock<mutex> lk(mtx);
            ++count;
            cv.notify_one();
        }
    };
}

#endif /* semaphore_hpp */
