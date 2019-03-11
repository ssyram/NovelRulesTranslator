//
//  TranslateException.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//


#define announce_with_default_s(name, super, default_s) class name: public super {\
public:\
name(const std::string &s = default_s): super(s) { }\
};

#define announce(name, super) class name: public super {\
public:\
name(const std::string &s): super(s) { }\
};

#ifndef TranslateException_h
#define TranslateException_h

#include <string>

namespace except {
    class StopMark { };
    
    class TranslateException {
    public:
        std::string message;
        TranslateException(const std::string &s): message(s) { }
    };
    
    announce_with_default_s(ConflictResolveCollisionTranslateException,
                            TranslateException,
                            "There are collision in the specifying of these productions")
}

#endif /* TranslateException_h */
