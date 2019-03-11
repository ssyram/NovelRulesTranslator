//
//  core.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef core_h
#define core_h

#include <memory>
#include "FileInteractor.h"
#include "../utils/PoolManager.hpp"
#include "objects.h"

namespace rules_translator {
    /**
     * to parse infomation from original file and also write infomation to the target file
     * params:
     *      fi: the object to interact with original file and target file
     *      cmd: the command passed from users
     *      logger: the logger that to print some log outside
     */
    std::shared_ptr<ParseInfo> parse(FileInteractor &fi, CommandObject &cmd, utils::Logger &logger);
    // to generate a counsel table to target file, returns the table generated
    // std::pair<pointer to action table, pointer to goto table>
    std::pair<std::shared_ptr<ActionTable>, std::shared_ptr<GotoTable>>
        generateTable(ParseInfo &info,
                      FileInteractor &fi,
                      CommandObject &cmd,
                      utils::Logger &logger,
                      utils::PoolManager &pool);
    // to generate the target analyzer to the target file
    void generateAnalyzer(FileInteractor &fi, CommandObject &cmd, utils::Logger &logger);
}

#endif /* core_h */
