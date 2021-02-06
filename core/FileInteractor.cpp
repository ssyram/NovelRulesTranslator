//
//  FileInteractor.cpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#include "../include/FileInteractor.h"
#include "../except/InnerTranslateException.h"
#include "../include/GlobalInfo.h"
#include "../except/ParseTranslateException.h"
#include "fi_info/fi_type.h"
#include "../utils/funcs.hpp"
#include "fi_info/fi_funcs.hpp"
#include "../utils/template_funcs.h"

using std::string;
using std::ostream;
using std::istream;
using std::function;
using namespace utils;

namespace rules_translator {
    
    const char * const type_string[] = { "nothing", "finished", "terminate", "identifier", "integer", "cpp_code", "cpp_block", "keyword_left", "keyword_right", "keyword_rank", "keyword_stable", "keyword_using", "keyword_properties", "keyword_front", "keyword_back", "keyword_nofront", "keyword_noback", "keyword_point", "sym_pro_equ", "sym_equ", "sym_semicolon", "sym_colon", "sym_comma", "sym_left_brace", "sym_right_brace", "sym_left_sqrbkt", "sym_right_sqrbkt", "sym_left_parenthesis", "sym_right_parenthesis", "sym_to", "sym_or", "sym_doc" };
    
    extern const char * const conflict_type_name_list[5] = { "", "REDUCE-SHIFT", "SHIFT-REDUCE", "", "REDUCE-REDUCE" };
    
    std::unordered_map<std::string, fi_type> reserved_word_map = {
        { "left", fi_type::keyword_left }, { "right", fi_type::keyword_right }, { "rank", fi_type::keyword_rank }, { "stable", fi_type::keyword_stable}, { "using", fi_type::keyword_using }, { "properties", fi_type::keyword_properties }, { "front", fi_type::keyword_front }, { "back", fi_type::keyword_back }, { "nofront", fi_type::keyword_nofront }, { "(", fi_type::sym_left_parenthesis }, { ")", fi_type::sym_right_parenthesis}, { "noback", fi_type::keyword_noback }, { "point", fi_type::keyword_point }, { ":=", fi_type::sym_pro_equ }, { "::=", fi_type::sym_pro_equ }, { "->", fi_type::sym_pro_equ }, { "=", fi_type::sym_equ }, { ";", fi_type::sym_semicolon }, { ":", fi_type::sym_colon }, { ",", fi_type::sym_comma }, { "{", fi_type::sym_left_brace }, { "}", fi_type::sym_right_brace }, { "[", fi_type::sym_left_sqrbkt }, { "]", fi_type::sym_right_sqrbkt }, { "|", fi_type::sym_or }, { ".", fi_type::sym_doc }
    };
    
    fi_info::fi_info() noexcept: type(fi_type::nothing), content("") { }
    fi_info::fi_info(fi_type type, const string &content) noexcept: type(type), content(content) { }
    
    class impl_file_reader {
        bool end = false, closed = false;
        Logger &logger;
        istream &ori;
        ostream &tar;
        function<void ()> clo_ori, clo_tar;
        const CommandObject &cmd;
        
        template <typename t, bool require, bool pre_jump>
        t inner_read(fi_func fmt) {
            constexpr auto b1 = std::is_same_v<std::decay_t<t>, std::decay_t<bool>>;
            
            constexpr auto b2 = std::is_same_v<std::decay_t<std::string>, std::decay_t<t>>;
            
            constexpr auto b3 = std::is_same_v<std::decay_t<t>, std::decay_t<fi_info>>;
            
            constexpr auto b4 = std::is_same_v<std::decay_t<t>, std::decay_t<fi_type>>;
            
            static_assert(b1 || b2 || b3 || b4, "Not a valid type.");
            
            
            if (end) {
                if constexpr (require)
                    throw except::UnexpectedEndParseException("The \"tsl block\" is not expected to end here.");
                else {
                    if constexpr (b1) return false;
                    if constexpr (b2) return "";
                    if constexpr (b3) return fi_info(fi_type::finished, "");
                    if constexpr (b4) return fi_type::finished;
                }
            }
            if (closed) throw except::ClosedInnerException("Trying to read a closed file.");
            
            fi_info info;
            
            if constexpr (pre_jump)
                fi_fn::object::get_over_divider_comment(info.type, info.content, ori);
            
            bool k = fmt(info.type, info.content, ori);
            
            if constexpr (require)
                if (!k) generateException<except::UnexpectedTokenParseException>("Unexpected Token", ori);
            
            if (ori.eof()) throw except::UnexpectedEndParseException();
            if (utils::expect(ori, "```"))
                end = true;
            
            if constexpr (b1)
                return k;
            else if constexpr (b2)
                return info.content;
            else if constexpr (b3)
                return info;
            else if constexpr (b4)
                return info.type;
        } // end member function inner_read<t, require, pre_jump>
        
    public:
        impl_file_reader(istream &ori, ostream &tar, Logger &logger, const CommandObject &cmd, function<void ()> clo_ori, function<void ()> clo_tar): ori(ori), tar(tar), logger(logger), cmd(cmd), clo_ori(clo_ori), clo_tar(clo_tar) {
            string buffer;
            while (true) {
                switch (find_until_s(buffer, ori, { "```tsl", "//", "/*", "\"", "'" })) {
                    case 0:
                        tar << buffer << std::endl;
                        return;
                    case 1:
                    case 2:
                        ori.seekg(-2, std::ios_base::cur);
                        get_comment(buffer, ori);
                        tar << buffer;
                        buffer.erase();
                        break;
                    case 3:
                    case 4:
                        ori.unget();
                        get_literal<true>(buffer, ori);
                        tar << buffer;
                        buffer.erase();
                        break;
                        
                    default:
                        throw except::UnexpectedEndParseException("THere is no \"tsl block\" in this file.");
                }
            }
        }
        
        ~impl_file_reader() {
            close();
        }
        
        template <bool require, bool pre_jump>
        fi_info read_info(fi_func &&fmt) {
            return inner_read<fi_info, require, pre_jump>(fmt);
        }
        
        // only returns string of fi_info
        template <bool require, bool pre_jump>
        std::string read(fi_func &&fmt) {
            return inner_read<string, require, pre_jump>(fmt);
        }
        
        // only returns fi_type of fi_info
        template <bool require, bool pre_jump>
        fi_type read_type(fi_func &&fmt) {
            return inner_read<fi_type, require, pre_jump>(fmt);
        }
        
        // returns if it has read something
        template <bool require, bool pre_jump>
        bool expect(fi_func &&fmt) {
            return inner_read<bool, require, pre_jump>(fmt);
        }
        
        void close() {
#ifdef TEST
            logger.log("Call close.");
#endif
            if (!end) throw except::ParseNotEndInnerException();
            if (closed) return;
            
            const static size_t BUFFER_SIZE = 1024;
            char *buffer = new char[BUFFER_SIZE];
            while (!ori.eof()) {
                ori.read(buffer, BUFFER_SIZE);
                tar.write(buffer, ori.gcount());
            }
            closed = true;
            delete [] buffer;
            clo_ori();
            clo_tar();
        }
        
        bool check_end() { return end; }
        bool check_close() { return closed; }
        std::istream &getOrigin() { return ori; }
        std::ostream &getTarget() { return tar; }
        utils::Logger &getLogger() { return logger; }
    };
}

using namespace rules_translator;

FileInteractor::FileInteractor
(std::istream &origin,
 std::ostream &target,
 utils::Logger &logger,
 const rules_translator::CommandObject &cmd,
 std::function<void ()> close_ori,
 std::function<void ()> close_tar): tar(target) {
    impl = new impl_file_reader(origin, target, logger, cmd, close_ori, close_tar);
}

FileInteractor::~FileInteractor() {
    delete impl;
}

void FileInteractor::close() {
    impl->close();
}

std::istream &FileInteractor::getOrigin() {
    return impl->getOrigin();
}

std::ostream &FileInteractor::getTarget() {
    return impl->getTarget();
}

utils::Logger &FileInteractor::getLogger() {
    return impl->getLogger();
}

fi_info FileInteractor::read_info(fi_func &&fmt, bool require, bool pre_jump) {
    if (pre_jump) {
        if (require) return impl->read_info<true, true>(std::move(fmt));
        else return impl->read_info<false, true>(std::move(fmt));
    }
    else if (require) return impl->read_info<true, false>(std::move(fmt));
    else return impl->read_info<false, false>(std::move(fmt));
}

fi_type FileInteractor::read_type(fi_func &&fmt, bool require, bool pre_jump) {
    if (pre_jump) {
        if (require) return impl->read_type<true, true>(std::move(fmt));
        else return impl->read_type<false, true>(std::move(fmt));
    }
    else if (require) return impl->read_type<true, false>(std::move(fmt));
    else return impl->read_type<false, false>(std::move(fmt));
}

bool FileInteractor::closed() {
    return impl->check_close();
}

bool FileInteractor::end() {
    return impl->check_end();
}

string FileInteractor::read(fi_func &&fmt, bool require, bool pre_jump) {
    if (pre_jump) {
        if (require) return impl->read<true, true>(std::move(fmt));
        else return impl->read<false, true>(std::move(fmt));
    }
    else if (require) return impl->read<true, false>(std::move(fmt));
    else return impl->read<false, false>(std::move(fmt));
}

bool FileInteractor::expect(fi_func &&fmt, bool require, bool pre_jump) {
    if (pre_jump) {
        if (require) return impl->expect<true, true>(std::move(fmt));
        else return impl->expect<false, true>(std::move(fmt));
    }
    else if (require) return impl->expect<true, false>(std::move(fmt));
    else return impl->expect<false, false>(std::move(fmt));
}


