//
//  PoolManager.hpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef PoolManager_hpp
#define PoolManager_hpp

#include <functional>

namespace utils {
    
    class impl_pool_manager;
    
    class PoolManager {
        impl_pool_manager *impl;
    public:
        // pool_size: the size of this pool
        // pay attention that the thread running to initialize PoolManager
        // is not managed by this pool
        PoolManager(size_t pool_size, bool &stop_signal);
        ~PoolManager();
        void execute(std::function<void()> to_run);
        bool has_empty(); // check if there is empty thread
    };
    
}


#endif /* PoolManager_hpp */
