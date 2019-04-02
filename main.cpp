//
//  main.cpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#include "run.h"

int main(int argc, const char * argv[]) {
    // insert code here...
	argc = 3;
	const char *a[] = { "1", "sample.tsl", "sample.hpp" };
	argv = a;
    auto r = run(argc, argv);
	int i = 5;
}
