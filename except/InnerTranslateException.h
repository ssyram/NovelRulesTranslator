//
//  InnerTranslateException.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef InnerTranslateException_h
#define InnerTranslateException_h

#include "TranslateException.h"

#define announce_this_with_default_s(name, default_s) announce_with_default_s(name, InnerTranslateException, default_s)
#define announce_this(name) announce(name, InnerTranslateException)

namespace except {
    class InnerTranslateException: public TranslateException {
    public:
        InnerTranslateException(const std::string &s): TranslateException(s) { }
    };
    
    class ClosedInnerException: public InnerTranslateException {
    public:
        ClosedInnerException(const std::string &s = "This FileInteractor has been closed!"):
            InnerTranslateException(s) { }
    };
    
    class ParseNotEndInnerException: public InnerTranslateException {
    public:
        ParseNotEndInnerException(const std::string &s = "The parsing process is not end!"):
            InnerTranslateException(s) { }
    };
    
    announce_this_with_default_s(IDEqualInnerException, "The pid set in priority setting must not equal to each other.")
    
    announce_this(NoSuchProductionInnerException)
    
    announce_this_with_default_s(ProductionWithoutProcessInnerException, "Every production must has its own semantic process.")
    
    announce_this(OutOfRangeInnerException)
    
    announce_this_with_default_s(AccessingInvalidSymbolInPWDInnerException, "This symbol is out of access range.")
    
    announce_this_with_default_s(TheSameShiftInnerException, "SHIFT to the same place is not acceptable")
    
#undef announce_with_default_s
#undef announce
#undef announce_this_with_default_s
#undef announce_this
    
}

#endif /* InnerTranslateException_h */
