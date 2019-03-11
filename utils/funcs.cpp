//
//  funcs.cpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#include "funcs.hpp"
#include "template_funcs.h"

namespace utils {
    
    bool is_divider(const char c) {
        const static std::unordered_set<char> set = {
            '\n', ' ', '\t', '\r'
        };
        return set.find(c) != set.end();
    }
    
    bool expect(std::istream &is, const std::string &tar) {
        if (is.eof()) return false;
        for (int i = 0; i < tar.size(); ++i)
            if (is.get() != tar[i]) {
                is.seekg(-i - 1, std::ios_base::cur);
                return false;
            }
        return true;
    }
    
    using std::string;
    using std::istream;
    
    string trim(const string &s, const string &tar) {
        string r(s);
        for (size_t pos = r.find(tar); pos != string::npos; pos = r.find(tar))
            r.erase(r.begin() + pos, r.begin() + pos + tar.size());
        return r;
    }
    
    string replace(const string &s, const string &o, const string &n) {
        string r(s);
        for (size_t pos = r.find(o); pos != string::npos; pos = r.find(o, pos + n.size()))
            r.replace(pos, o.size(), n);
        return r;
    }
    
    bool find_until(istream &is, const string &tar) {
        if (tar.empty()) return !is.eof();
        if (tar.size() == 1) {
            is.ignore(std::numeric_limits<std::streamsize>::max(), tar[0]);
            return true;
        }
        
        list<size_t> ps;
        while (!is.eof()) {
            char c = is.get();
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
        return true;
    }

    
    bool is_number(const char c) {
        return (unsigned char)(c - '0') < 10;
    }
    
    bool get_comment(string &s, istream &is) {
        if (is.get() != '/') goto ret;
        
        if (is.peek() == '/') {
            s += '/';
            while (!find_until_s<true>(s, is, { "\\", "\n" })) {
                if (is.peek() == '\n') s += is.get();
            }
        }
        else if (is.peek() == '*') {
            is.get();
            s += "/*";
            string buffer;
            while (std::getline(is, buffer, '*')) {
                s += buffer;
                if (is.peek() == '/') goto brk;
                s += '*';
            }
            throw "Unterminated /* comment";
        brk:
            is.get();
            s += "*/";
        }
        else goto ret;
        return true;
    ret:
        is.unget();
        return false;
    }
    
    bool pass_through_comment(istream &is) {
        if (is.get() != '/') goto ret;
        
        if (is.peek() == '/')
            while (!find_until_s(is, { "\\", "\n" })) {
                if (is.peek() == '\n') is.get();
            }
        else if (is.peek() == '*') {
            is.get();
            while (is.ignore(std::numeric_limits<std::streamsize>::max(), '*'))
                if (is.peek() == '/') goto brk;
            throw "Unterminated /* comment";
        brk:
            is.get();
            return true;
        }
        else goto ret;
        return true;
    ret:
        is.unget();
        return false;
    }
    
    using ll = long long;
    
    ll string2ll(const string &s) {
        if (s.empty()) return 0;
        bool minus = false;
        ll r = 0;
        size_t i = 0;
        if (s[0] == '-') {
            minus = true;
            ++i;
        }
        
        for (; i < s.size(); ++i)
            r = r * 10 + (s[i] - '0');
        
        return minus ? -r : r;
    }
    
    size_t find_until_s(istream &is, const vector<string> &tars) {
        if (tars.empty()) return -1;
        size_t idx = 0;
        for (const auto &s: tars) {
            if (s.empty()) return is.eof() ? -1 : idx;
            ++idx;
        }
        list<pair<size_t, size_t>> ps;
        while (!is.eof()) {
            char c = is.get();
            for (auto it = ps.begin(); it != ps.end(); )
                if (c == tars[it->second][it->first]) {
                    ++(it->first);
                    if (it->first == tars[it->second].size()) return it->second;
                    ++it;
                }
                else it = ps.erase(it);
            idx = 0;
            for (const auto &s: tars) {
                if (c == s[0]) {
                    if (s.size() == 1) return idx;
                    else ps.push_back(std::make_pair(1, idx));
                }
                ++idx;
            }
        }
        return -1;
    } // end function find_until_s
    
    bool is_alphabet(const char c) {
        return (unsigned char)((c - 'A') % 32) < 26;
    }
    
    bool can_in_word(const char c) {
        return (unsigned char)((c - 'A') % 32) < 26 || (unsigned char)(c - '0') < 10 || c == '_';
    }
    
    size_t string2size_t(const string &s) {
        size_t r = 0;
        for (char c: s)
            r = r * 10 + (c - '0');
        return r;
    }
    
    size_t set_pos(size_t entity, uint8_t start_pos, uint8_t length) {
        return (entity << (64 - length)) >> (64 - length + start_pos);
    }
    
    string generate_pos_info(istream &is) {
        
        size_t pos = is.tellg();
        char info[128];
        is.seekg(-64, std::ios_base::cur).read(info, 64);
        // mark this place
        info[64] = '('; info[65] = '^'; info[66] = ')';
        is.read(info + 67, 60);
        info[127] = '\0';
        is.seekg(pos);
        
        return string(info);
    }
}
