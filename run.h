//
//  run.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/3/10.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef run_h
#define run_h

#include "include/core.h"

/**
 * the entrance of this project
 * analyze arguments passed to the program
 * then run the analysis
 *
 * About command format:
 *      <./program> "path_to_tsl_file" ["path"]
 */
int run(int argc, const char *argv[]);

#endif /* run_h */
