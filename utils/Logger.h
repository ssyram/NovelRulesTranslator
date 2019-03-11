//
//  Logger.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef Logger_h
#define Logger_h

#include <iostream>
#include <string_view>
#include <unordered_set>

namespace utils {
    using std::string;
    using std::istream;
    using std::ostream;
    using std::string_view;
    using std::endl;
    using std::unordered_set;
    
    struct interrupt_mark { const string &msg; };
    
    class Logger {
        ostream &normal, &error, &warning;
    public:
        Logger(ostream &normal, ostream &error, ostream &warning): normal(normal), warning(warning), error(error) { }
        unordered_set<string> interrupt_strings;
        
#define func_set(type_name, __where)\
        template <typename ...Args>\
        Logger &type_name(Args &&...msgs) {\
            (__where << ... << msgs);\
            return *this;\
        }\
        template <typename ...Args>\
        Logger &type_name##ln(Args &&...msgs) {\
            (__where << ... << msgs) << endl;\
            return *this;\
        }
        
        func_set(log, normal)
        func_set(err, error)
        func_set(warn, warning)
        
#undef func_set
        void log_with_is(const string_view &msg, istream &is) {
#pragma message("Not finish.")
        }
        void warn(const string_view &msg) {
#pragma message("Not finish.")
        }
    };
}

#endif /* Logger_h */
