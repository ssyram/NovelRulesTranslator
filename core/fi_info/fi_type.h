//
//  fi_type.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef fi_type_h
#define fi_type_h



namespace rules_translator {
    
    enum class fi_type {
        nothing,
        finished,
        terminate,
        identifier,
        integer,
        cpp_code,
        cpp_block,
        keyword_left,
        keyword_right,
        keyword_rank,
        keyword_stable,
        keyword_using,
        keyword_properties,
        keyword_front,
        keyword_back,
        keyword_nofront,
        keyword_noback,
        keyword_point,
        sym_pro_equ,
        sym_equ,
        sym_semicolon,
        sym_colon,
        sym_comma,
        sym_left_brace,
        sym_right_brace,
        sym_left_sqrbkt,
        sym_right_sqrbkt,
        sym_left_parenthesis,
        sym_right_parenthesis,
        sym_to,
        sym_or,
        sym_doc
    };
    
    extern const char * const type_string[];
    
}

#endif /* fi_type_h */
