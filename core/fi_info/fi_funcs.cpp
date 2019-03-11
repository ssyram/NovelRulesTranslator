//
//  fi_funcs.cpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#include "fi_funcs.hpp"
#include "../../except/InnerTranslateException.h"
#include "../../except/ParseTranslateException.h"
#include "../../utils/funcs.hpp"
#include "../../utils/template_funcs.h"

// this file defines some functions that are going to be used as "fmt" by FileInteractor
// all of the functions that need istream& follows a simple rule:
// read to the char that cannot be accepted by this function
namespace rules_translator::fi_fn {
    
    static fi_type get_sym_by_istream(istream &is) {
        char temp = is.get();
        switch (temp) {
            case '.':
                return fi_type::sym_doc;
            case '-':
                if (is.get() == '>') return fi_type::sym_pro_equ;
                is.unget();
                return fi_type::sym_to;
            case ':':
                if ((temp = is.get()) == ':') {
                    if ((temp = is.get()) == '=')
                        return fi_type::sym_pro_equ;
                    else {
                        is.seekg(-2, std::ios_base::cur);
                        return fi_type::sym_colon;
                    }
                }
                else if (temp == '=')
                    return fi_type::sym_pro_equ;
                else {
                    is.unget();
                    return fi_type::sym_colon;
                }
            case ',':
                return fi_type::sym_comma;
            case '=':
                return fi_type::sym_equ;
            case ';':
                return fi_type::sym_semicolon;
            case '{':
                return fi_type::sym_left_brace;
            case '}':
                return fi_type::sym_right_brace;
            case '[':
                return fi_type::sym_left_sqrbkt;
            case ']':
                return fi_type::sym_right_sqrbkt;
            case '|':
                return fi_type::sym_or;
            case '(':
                return fi_type::sym_left_parenthesis;
            case ')':
                return fi_type::sym_right_parenthesis;
                
            default:
                is.unget();
                return fi_type::nothing;
        }
    }
    
    namespace object {
        
#define fi(name) bool name(fi_type &type, std::string &s, istream &is)
        
        fi(nothing) {
            return false;
        }
        
        fi(get_literal) {
            bool r;
            if ((r = utils::get_literal(s, is)))
                type = fi_type::terminate;
            return r;
        }
        
        fi(get_identifier) {
            if (!(utils::is_alphabet(is.peek()) || is.peek() == '_')) return false;
            string buffer("");
            do
                buffer += is.get();
            while (utils::can_in_word(is.peek()));
            
            auto f = reserved_word_map.find(buffer);
            if (f == reserved_word_map.end()) {
                s += buffer;
                type = fi_type::identifier;
            }
            else type = f->second;
            
            return true;
        }
        
        fi(get_sym) {
            fi_type k;
            if ((k = get_sym_by_istream(is)) == fi_type::nothing)
                return false;
            type = k;
            return true;
        }
        
        fi(get_integer) {
            if (!utils::is_number(is.peek())) {
                if (is.peek() == '-') {
                    is.get();
                    if (!utils::is_number(is.peek())) {
                        is.unget();
                        return false;
                    }
                }
                else return false;
            }
            
            do s += is.get(); while (utils::is_number(is.peek()));
            type = fi_type::integer;
            
            return true;
        }
        
        fi(get_unsigned_integer) {
            if (!utils::is_number(is.peek())) return false;
            
            do s += is.get(); while (utils::is_number(is.peek()));
            type = fi_type::integer;
            
            return true;
        }
        
        fi(get_formatted_cpp_code_to_semicolon) {
            if (is.peek() == ';') return false;
            
            type = fi_type::cpp_code;
            
            string buffer;
            
            bool multi_divider = false;
            for (char c = is.get(); c != ';' && !is.eof(); c = is.get()) {
                if (c == '/') {
                    is.unget();
                    if (!utils::pass_through_comment(is)) {
                        buffer += is.get();
                        multi_divider = false;
                    }
                    else if (!multi_divider) {
                        multi_divider = true;
                        buffer += ' ';
                    }
                }
                else if (utils::is_divider(c)) {
                    if (!multi_divider) {
                        multi_divider = true;
                        buffer += ' ';
                    }
                    //                    if (multi_divider)
                    //                        continue;
                    //                    else {
                    //                        multi_divider = true;
                    //                        s += ' ';
                    //                        continue;
                    //                    }
                }
                else if (c == '\'' || c == '"') {
                    is.unget();
                    utils::get_literal<true>(buffer, is);
                    multi_divider = false;
                }
                else {
                    multi_divider = false;
                    buffer += c;
                }
            }
            
            if (buffer[0] == ' ')
                buffer.erase(buffer.begin(), buffer.begin() + 1);
            if (buffer.empty()) return false;
            if (buffer.back() == ' ')
                buffer.erase(buffer.end() - 1, buffer.end());
            
            // for that there would be no continuous dividers, so here the content will not be empty
            
            s += buffer;
            
            return true;
        }
        
        fi(get_cpp_block) {
            type = fi_type::cpp_block;
            int count = 0;
            string buffer;
            {
                string &s = buffer;
                for (char c = is.get(); !is.eof(); c = is.get()) {
                    if (c == '/') {
                        is.unget();
                        if (utils::pass_through_comment(is)) s += ' ';
                        else s += is.get();
                    }
                    else if (c == '\'' || c == '"') {
                        is.unget();
                        utils::get_literal<true>(s, is);
                    }
                    else if (c == '{') {
                        ++count;
                        s += c;
                    }
                    else if (c == '}') {
                        --count;
                        if (count < 0) goto ret;
                        s += c;
                    }
                    else if (c == '\n' || c == '\r') {
                        while (is.peek() == ' ' || is.peek() == '\t') is.get();
                        s += c;
                    }
                    else s += c;
                }
                throw except::UnexpectedEndParseException("Unterminated semantic process block.");
            }
            
        ret:
            // remove front and back dividers
            {
                auto it = buffer.begin();
                while (it != buffer.end() && utils::is_divider(*it)) ++it;
                buffer.erase(buffer.begin(), it);
                if (buffer.empty()) return true;
                it = buffer.end();
                --it;
                while (utils::is_divider(*it)) --it;
                buffer.erase(++it, buffer.end());
            }
            s += buffer;
            return true;
        }
        
#undef fi
        
    }
#define func_first (fi_type &type, std::string &s, istream &is) -> bool
    namespace generator {
        
        // do not need to receive base "fmt"
        fi_func expect(const string &content) {
            return [content](fi_type &type, std::string &s, istream &is) -> bool {
                return utils::expect(is, content);
            };
        }
        
            
#define fi(name) fi_func name(fi_func &&fmt) {\
return [fmt] (fi_type &type, std::string &s, istream &is) -> bool {\
if (fmt(type, s, is)) return true;\
return object::name(type, s, is);\
};\
}
        
        fi(get_literal)
        
        fi(get_identifier)
        
        fi(get_sym)
        
        fi(get_integer)
        
        fi(get_unsigned_integer)
        
        fi(get_formatted_cpp_code_to_semicolon)
        
        fi(get_cpp_block)
        
#undef fi
        
        
    }
    
    namespace container {
        fi_func sequence(const vector<fi_func> &fmt) {
            return [fmt] func_first {
                for (auto &f: fmt)
                    if (f(type, s, is)) return true;
                return false;
            };
        }
        
    }
    
    namespace compound {
        
    }
#undef func_first
    // some easy interfaces that can be called easily by just specifying the elements that needed
    namespace easy_interface {
        enum class element {
            
        };
    }
}
