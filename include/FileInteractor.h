//
//  FileInteractor.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef FileInteractor_h
#define FileInteractor_h

#include <functional>
#include <string>
#include <string_view>
#include <iostream>
#include <unordered_map>
#include "objects.h"
#include "../utils/Logger.h"

namespace rules_translator {
    
    enum class fi_type;
    
    /**
     * kinds: bool (*fi_func)(fi_type &type, std::string &s, std::istream &is);
     * descriptions:
     *      Read a token from (std::istream &)"is" and append it to (std::string &)"s",
     *      at the same time, specify (rules_translator::core::fi_fype &)"type" to tell
     *      how to interpret (std::string &)"s" and return if it has finished reading a
     *      token.
     *
     * rules:
     *      1. if read a keyword of NovelRulesTranslator, s.empty() should return true.
     *      2. if returns false, which means read nothing,
     *         (type == fi_type::nothing && s.empty()) should return true.
     *         that is: if read nothing, return value should synchronize with type and s.
     *      ** the rules above should only apply to interface functions, which means the
     *      ** ones that be passed into checking functions, tool functions may not obey
     *      ** these rules.
     *      ** checking functions are those who will check the constancy of the three values
     *      ** and should be marked by its comment.
     *
     * params:
     *      type: how to interpret "s".
     *      s: the string content read from "is".
     *      is: the source to read from.
     *
     * returns:
     *      if the function has finished reading, return true, else return false
     */
    using fi_func = std::function<bool (fi_type &, std::string &, std::istream &)>;
    
    extern std::unordered_map<std::string, fi_type> reserved_word_map;
    
    // the struct that is the return value type of member functions of FileInteractor
    struct fi_info {
        fi_type type;
        std::string content;
        fi_info() noexcept;
        fi_info(fi_type type, const std::string &content) noexcept;
    };
    
    class impl_file_reader;
    
    /**
     * This class takes the responsibility of interacting with files
     * this part is to be passed into core functions
     */
    class FileInteractor {
        impl_file_reader *impl;
        // this function is the default function of close function(s)
        std::ostream &tar;
        static void nothing() { }
    public:
        /**
         * params:
         *      origin: original file that to be read from
         *      target: the target file to write to
         *      logger: help to log what happened inside
         *      cmd: the specified command
         *      close_ori: the function to specify how to close original file
         *      close_tar: the function to specify how to close target file
         */
        FileInteractor(
                       std::istream &origin,
                       std::ostream &target,
                       utils::Logger &logger,
                       const CommandObject &cmd,
                       std::function<void ()> close_ori = nothing,
                       std::function<void ()> close_tar = nothing
                       );
        ~FileInteractor();
        
        // read functions
        /**
         * description:
         *      require: if it's required, means that if read nothing, it should
         *               throw exceptions right away
         *      pre_jump: to tell if the function should jump to next meaningful
         *                character first, to avoid comments and dividers
         *
         *      All of the following four functions will execute "fmt" and returns
         *      its information, they only differ from each other by return value
         *
         *      These are all checking functions that requires value returns from
         *      "fmt" to obey the rules of return value, "type" and "s"
         */
        // returns fi_info as a result of "fmt" execution
        fi_info read_info(fi_func &&fmt, bool require = true, bool pre_jump = true);
        
        // only returns string of fi_info
        std::string read(fi_func &&fmt, bool require = true, bool pre_jump = true);
        
        // only returns fi_type of fi_info
        fi_type read_type(fi_func &&fmt, bool require = true, bool pre_jump = true);
        
        // returns if it has read something
        bool expect(fi_func &&fmt, bool require = true, bool pre_jump = true);
        
        // write functions
        /**
         * returns "*this"
         */
        template <typename ...Args>
        FileInteractor &write(Args &&...content) {
            (tar << ... << content);
            return *this;
        }
        template <typename ...Args>
        FileInteractor &writeln(Args &&...content) {
            (tar << ... << content) << std::endl;
            return *this;
        }
        
        // return whether it has closed.
        bool closed();
        // return if it has ended
        bool end();
        // functional
        // to close, normally, there is no need to call this function manually
        void close();
        std::istream &getOrigin();
        std::ostream &getTarget();
        utils::Logger &getLogger();
    }; // end class FileInteractor
    
}

#endif /* FileInteractor_h */
