//
//  funcs.hpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef funcs_hpp
#define funcs_hpp

#include <istream>
#include <string>
#include <unordered_set>
#include <sstream>
#include <list>
#include <vector>

namespace utils {
    
    bool is_divider(const char c);
    
    bool expect(std::istream &is, const std::string &tar);
    
    using std::string;
    using std::istream;
    
    string trim(const string &s, const string &tar);
    
    // remove all comment and format all dividers
    string format(const string &s);
    
    // checked
    // returns if it is comment
    bool get_comment(string &s, istream &is);
    
    // checked
    // returns if it is comment
    // when it comes to eof(), if it's line, returns true
    // if it's "/**/", returns false
    bool pass_through_comment(istream &is);
    
    using std::list;
    
    // checked
    bool find_until(istream &is, const string &tar);
    
    using std::vector;
    using std::pair;
    
    // checked
    size_t find_until_s(istream &is, const vector<string> &tars);
        
    bool is_alphabet(const char c);
    
    bool can_in_word(const char c);
    
    long long string2ll(const string &s);
    
    size_t string2size_t(const string &s);
    
    using std::stringstream;
    
    bool is_number(const char c);
    
    // to help hash
    size_t set_pos(size_t entity, uint8_t start_pos, uint8_t length);
    
    string generate_pos_info(istream &is);
    
}


#endif /* funcs_hpp */
