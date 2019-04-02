//
//  GlobalInfo.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef GlobalInfo_h
#define GlobalInfo_h

#include <string>
#include <istream>
#include <type_traits>
#include <iostream>
#include <assert.h>

template <typename ttr>
void generateException(const std::string &s, std::istream &is) {
    std::string rs(s);
    rs += ": ";
    
    char info[128];
    is.seekg(-64, std::ios_base::cur).read(info, 64);
    // mark this place
    info[64] = '('; info[65] = '^'; info[66] = ')';
    is.read(info + 67, 60);
    info[127] = '\0';
    
    if (is.fail()) {
        std::cout << "Fail" << std::endl;
        is.unget();
    }
    else if (is.bad()) {
        std::cout << "Bad" << std::endl;
    }
    
    throw ttr(rs += info);
}

#endif /* GlobalInfo_h */
