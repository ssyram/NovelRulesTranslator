//
//  fi_funcs.hpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef fi_funcs_hpp
#define fi_funcs_hpp

#include "../../include/FileInteractor.h"
#include "fi_type.h"
#include "../../utils/funcs.hpp"
#include "../../utils/template_funcs.h"
#include "../../except/ParseTranslateException.h"
#include <string>
#include <istream>
#include <functional>
#include <vector>

// this file defines some functions that are going to be used as "fmt" by FileInteractor
// all of the functions that need istream& follows a simple rule:
// read to the char that cannot be accepted by this function
namespace rules_translator::fi_fn {
    
    using std::string;
    using std::istream;
    
    using fi_func = std::function<bool (fi_type &, string &, istream &)>;
    
    namespace object {
        
#define fi(name) bool name(fi_type &type, std::string &s, istream &is)
        
        fi(nothing);
        
        // checked
        template <bool get_divider = false, bool get_comment = false>
        fi(get_over_divider_comment) {
            int k = 0;
        cont:
            ++k;
            if (utils::is_divider(is.peek())) {
                if constexpr (get_divider) s += is.get();
                else is.get();
                goto cont;
            }
            if constexpr (get_comment) {
                if (utils::get_comment(s, is)) goto cont;
            }
            else
                if (utils::pass_through_comment(is)) goto cont;
            
            if constexpr (get_divider || get_comment) {
                if (k > 1) return true;
                else return false;
            }
            else
                return false;
        }
        
        fi(get_literal);
        
        fi(get_identifier);
        
        fi(get_sym);
        
        fi(get_integer);
        
        fi(get_unsigned_integer);
        
        fi(get_formatted_cpp_code_to_semicolon);
        
        fi(get_cpp_block);
        
#undef fi
        
    }
    
#define func_first (fi_type &type, std::string &s, istream &is) -> bool
    namespace generator {
        
        // do not need to receive base "fmt"
        /**
         * returns a function that check whether the next string is the specified content
         */
        fi_func expect(const string &content);
        template <bool it = false>
        fi_func get_until(const string &content) {
            return [content] func_first {
                return utils::find_until(s, is, content);
            };
        }
        
#define fi(name) fi_func name(fi_func &&fmt)
        
        fi(get_identifier);
        
        fi(get_literal);
        
        fi(get_identifier);
        
        fi(get_sym);
        
        fi(get_integer);
        
        fi(get_unsigned_integer);
        
        fi(get_formatted_cpp_code_to_semicolon);
        
        fi(get_cpp_block);
        
#undef fi
        
    }
    
    using std::vector;
    
    namespace container {
        fi_func sequence(const vector<fi_func> &fmt);
        // check_match means to check if read something
        // is it the target type
        template <bool check_match = false>
        fi_func expect(fi_func &&fmt, const fi_type &ft) {
            return [ft, fmt] func_first {
                const size_t cur = is.tellg();
                if (!fmt(type, s, is)) return false;
                if (ft == type) return true;
                if constexpr (!check_match) {
                    is.seekg(cur);
                    s.erase();
                    type = fi_type::nothing;
                }
            fret:
                if constexpr (!check_match) return false;
                string ts("Expected token: ");
                ts += type_string[uint16_t(ft)];
                generateException
                    <except::NotGetExpectedTokenParseException>(ts, is);
                return false;
            };
        }
        template <bool check_match = false>
        fi_func expect(fi_func &&fmt, const vector<fi_type> &fts) {
            return [fmt, fts] func_first {
                if (fts.empty()) throw except::InnerTranslateException("Cannot expect nothing.");
                const size_t cur = is.tellg();
                if (!fmt(type, s, is)) return false;
                for (fi_type ft: fts)
                    if (ft == type) return true;
                if constexpr (!check_match) {
                    is.seekg(cur);
                    s.erase();
                    type = fi_type::nothing;
                }
            fret:
                if constexpr (!check_match) return false;
                string ts("Expected token(s): ");
                for (fi_type ft: fts)
                    (ts += type_string[int(ft)]) += ", ";
                generateException
                    <except::NotGetExpectedTokenParseException>(ts, is);
                return false;
            };
        }
    }
    
    namespace compound {
        
    }
#undef func_first
    
    // some easy interfaces that can be called easily by just specifying the elements that needed
    namespace easy_interface {
        enum class element;
    }
    
}

#endif /* fi_funcs_hpp */
