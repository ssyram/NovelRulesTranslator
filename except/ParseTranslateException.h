//
//  ParseTranslateException.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef ParseTranslateException_h
#define ParseTranslateException_h

#include "TranslateException.h"

#define announce_this_with_default_s(name, default_s) announce_with_default_s(name, ParseTranslateException, default_s)
#define announce_this(name) announce(name, ParseTranslateException)

namespace except {
    class ParseTranslateException: public TranslateException {
    public:
        ParseTranslateException(const std::string &s): TranslateException(s) { }
    };
    
    announce_this_with_default_s(UnexpectedEndParseException, "The \"tsl block\" is not expected to end here.")
    
    announce_this(UnexpectedTokenParseException)
    
    announce_this(UnknownTokenParseException)
    
    announce_this(NotGetExpectedTokenParseException)
    
    announce_this(DoubleNameParseException)
    
    announce_this(DoubleSetStrongCombinationParseException)
    
    announce_this(SpecifyingConflictParseException)
    
    announce_this(EmptyBlockParseException)
    
    announce_this(InvalidIdRangeParseException)
    
    announce_this(CallToNotEndBlockParseException)
    
    announce_this(InvalidPointTypeParseException)
    
    announce_this(DoubleProductionParseException)
    
    announce_this(TokenNotRegisteredParseException)
    
    announce_this(BlockTypeNotMatchParseException)
    
    announce_this(DoubleDuplicationParseException)
    
    announce_this(UnterminatedLiteralParseException)
}

#undef announce_with_default_s
#undef announce
#undef announce_this_with_default_s
#undef announce_this

#endif /* ParseTranslateException_h */
