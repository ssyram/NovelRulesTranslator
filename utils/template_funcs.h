//
//  template_funcs.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef template_funcs_h
#define template_funcs_h

#include <string>
#include <istream>
#include <vector>

namespace utils {
    
    // checked
    // get a literal that embraces by '\'' or '"'
    // must start with these two, if not, returns false
    // the content will be appended to "s" from "is"
    // checked
    template <bool get_quote = false>
    bool get_literal(string &s, istream &is) {
        const char k = is.peek();
        if (k != '\'' && k != '"') return false;
        
        if constexpr (get_quote) s += is.get();
        else is.get();
        
        bool ignore = false;
        for (char i = is.peek(); !is.eof() && (ignore || i != k); i = is.peek()) {
            if (i == '\\' && !ignore)
                ignore = true;
            else ignore = false;
            
            s += is.get();
        }
            
        if (is.eof()) return false;
        
        is.get();
        if constexpr (get_quote) s += k;
        
        return true;
    }
    
    // checked
    /**
     * description:
     *      find the target string from "is", returns whether the target string is found.
     *
     * margin condition:
     *      if source has gone to end (is.eof() == true), returns false;
     *      if target string is empty (tar.empty() == true), returns if source has ended.
     *
     * template:
     *      get_tar: whether to let tar appear in "s". true for yes, false for no.
     *
     * params:
     *      s: the container to get the path to "tar".
     *      is: the source of getting.
     *      tar: the target string
     */
    template <bool get_tar = false>
    bool find_until(string &s, istream &is, const string &tar) {
        if (tar.empty()) return !is.eof();
        if (tar.size() == 1) {
            string buffer;
            std::getline(is, buffer, tar[0]);
            s += buffer;
            if (is.eof()) return false;
            if constexpr (get_tar) s += tar;
            return true;
        }
        
        list<size_t> ps;
        while (!is.eof()) {
            char c = is.get();
            s += c;
            for (auto it = ps.begin(); it != ps.end(); )
                if (c == tar[*it]) {
                    ++(*it);
                    if (*it == tar.size()) goto brk;
                    ++it;
                }
                else it = ps.erase(it);
            if (c == tar[0]) ps.push_back(1);
        }
        return false;
    brk:
        if constexpr (!get_tar)
            s.erase(s.end() - tar.size(), s.end());
        return true;
    }
    
    using std::vector;
    using std::pair;
    
    // returns the position in which the target string is in the "tars" vector.
    // if none, returns -1, so when "tars" is empty, returns -1
    template <bool get_tar = false>
    size_t find_until_s(string &s, istream &is, const vector<string> &tars) {
        if (tars.empty()) return -1;
        size_t idx = 0;
        for (const auto &s: tars) {
            if (s.empty()) return is.eof() ? -1 : idx;
            ++idx;
        }
        list<pair<size_t, size_t>> ps; // pair<position pointer, tar index>
        while (!is.eof()) {
            char c = is.get();
            s += c;
            for (auto it = ps.begin(); it != ps.end(); )
                if (c == tars[it->second][it->first]) {
                    ++(it->first);
                    if (it->first == tars[it->second].size()) {
                        idx = it->second;
                        goto brk;
                    }
                    ++it;
                }
                else it = ps.erase(it);
            idx = 0;
            for (const auto &s: tars) {
                if (c == s[0]) {
                    if (s.size() == 1) goto brk;
                    else ps.push_back(std::make_pair(1, idx));
                }
                ++idx;
            }
        }
        return -1;
    brk:
        if constexpr (!get_tar)
            s.erase(s.end() - tars[idx].size(), s.end());
        return idx;
    } // end function find_until_s
    
}

#endif /* template_funcs_h */
