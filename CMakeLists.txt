cmake_minimum_required(VERSION 3.12)
project(NovelRulesTranslator)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)
include_directories(core)
include_directories(core/fi_info)
include_directories(except)
include_directories(extern)
include_directories(extern/files)
include_directories(include)
include_directories(utils)

add_executable(NovelRulesTranslator
        core/fi_info/fi_funcs.cpp
        core/fi_info/fi_funcs.hpp
        core/fi_info/fi_type.h
        core/AnalyzerGenerator.cpp
        core/AnalyzerGenerator.hpp
        core/FileInteractor.cpp
        core/InfoParser.cpp
        core/InfoParser.hpp
        core/TableGenerator.cpp
        core/TableGenerator.hpp
        except/InnerTranslateException.h
        except/ParseTranslateException.h
        except/TranslateException.h
        extern/files/TargetDependence.h
        include/core.h
        include/FileInteractor.h
        include/GlobalInfo.h
        include/objects.h
        include/periphery.h
        utils/funcs.cpp
        utils/funcs.hpp
        utils/Logger.h
        utils/PoolManager.cpp
        utils/PoolManager.hpp
        utils/semaphore.hpp
        utils/template_funcs.h
        main.cpp
        run.cpp
        run.h)
