//
//  run.cpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/3/10.
//  Copyright © 2019 ssyram. All rights reserved.
//

#include "extern/files/TargetDependence.h"
#include "run.h"
#include <unordered_map>
#include <string>
#include <optional>
#include <iostream>
using std::cout;
using std::endl;
using std::optional;
using std::unordered_map;
using std::string;

unordered_map<string, size_t> t_map = {
    { "-no_parse_range_warn", 2 },
    { "-restrict_nonformat_terminate", 3 },
    { "-ignore_duplicate_production", 4 },
    { "-ignore_anonym_for_cr", 5 },
    { "-property_list_for_auto_generate", 6 },
    { "-ignore_batch_for_anonym", 7 },
    { "-show_all_collisions", 8 },
    { "-o", 9 },
    { "-l", 10 },
    { "thread", 11 }
};

struct CommandParseObject {
    bool
    no_parse_range_warn = false,
    restrict_nonformat_terminate = false,
    ignore_duplicate_production = false,
    ignore_anonym_for_cr = false,
    property_list_for_auto_generate = false,
    ignore_batch_for_anonym = false,
    show_all_collisions = false;
    string tsl_file_path;
    optional<string> target_file_path, log_path;
    optional<size_t> thread_amount;
};

CommandParseObject cpo;

using CommandParseException = string;

#define check_reset(name) if (info.name) throw CommandParseException("Reset for name");

namespace runner {
    
    using token_type = string;size_t get_type(const token_type &token) {
        for (auto c: token)
            if (unsigned(c - '0') >= 10) goto ret;
        return 1;
    ret:
        const auto it = t_map.find(token);
        if (it != t_map.end()) return it->second;
        return 0;
    }
    using pass_info = CommandParseObject;
    struct default_object_type{ };struct iter_object_type;using object_type = std::variant<default_object_type, token_type, iter_object_type>;
    namespace utils { extern const std::string * const type_name_map; }
    using ll = long long;
    struct symbol_type;struct iter_object_type {std::vector<symbol_type> data;};
    struct symbol_type{
        ll type; object_type object;
        symbol_type() = default;
        symbol_type(ll type, object_type &&object): type(type), object(std::move(object)) { }
        symbol_type&operator=(symbol_type &&s) {
            type = s.type; object = std::move(s.object); return *this; };
        void operator=(const symbol_type &) = delete;
        void operator=(symbol_type &) = delete;
        symbol_type(const symbol_type &) = default;
    };
    template <typename ...Ts> struct overloaded: Ts... { using Ts::operator() ...; };
    template <typename ...Ts> overloaded(Ts...) -> overloaded<Ts...>;
    symbol_type __default_object_type_semantic_process_(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        return symbol_type{ left_type, default_object_type{} };
    }
    symbol_type __initial_auto_generated_acproduction_(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        iter_object_type left;
        for (auto it = right.begin(); it != right.end(); ++it)
            left.data.emplace_back(*it);
        return symbol_type{ left_type, std::move(left) };
    }
    symbol_type __left_combine_auto_generated_acproduction_(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        iter_object_type left{std::move(std::get<iter_object_type>(right.front().object).data)};
        for (auto it = right.begin() + 1; it != right.end(); ++it)
            left.data.emplace_back(*it);
        return symbol_type{ left_type, std::move(left) };
    }
    symbol_type __right_combine_auto_generated_acproduction_(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        iter_object_type left{std::move(std::get<iter_object_type>(right.back().object).data)};
        const auto END = right.end() - 1;
        for (auto it = right.begin(); it != END; ++it)
            left.data.emplace_back(*it);
        return symbol_type{ left_type, std::move(left) };}
    symbol_type __process_4(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](token_type&$1, default_object_type&$$) {
            if (!info.tsl_file_path.empty()) {
                if (info.target_file_path)
                    throw CommandParseException("Reset for target file path");
                info.target_file_path = $1;
            }
            else info.tsl_file_path = $1;
        }, [](auto &&, auto &&) { assert(false); }
        }, right[0].object, left.object);
        return left;
    }
    symbol_type __process_5(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](token_type&$2, default_object_type&$$) {
            if (info.target_file_path)
                throw CommandParseException("Reset for target file path");
            info.target_file_path = $2;
        }, [](auto &&, auto &&) { assert(false); }
        }, right[1].object, left.object);
        return left;
    }
    symbol_type __process_6(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](token_type&$2, default_object_type&$$) {
            check_reset(log_path)
            info.log_path = $2;
        }, [](auto &&, auto &&) { assert(false); }
        }, right[1].object, left.object);
        return left;
    }
    symbol_type __process_7(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](token_type&$2, default_object_type&$$) {
            if (info.thread_amount)
                throw CommandParseException("Reset for thread_amount");
            auto &ts = *info.thread_amount;
            ts = 0;
            for (auto c: $2)
                ts = ts * 10 + c - '0';
            if (!ts || ts > 100)
                throw CommandParseException("Not a valid thread amount specification");
        }, [](auto &&, auto &&) { assert(false); }
        }, right[1].object, left.object);
        return left;
    }
    symbol_type __process_8(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](default_object_type&$$) {
            check_reset(no_parse_range_warn) info.no_parse_range_warn = true;
        }, [](auto &&) { assert(false); }
        }, left.object);
        return left;
    }
    symbol_type __process_9(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](default_object_type&$$) {
            check_reset(restrict_nonformat_terminate) info.restrict_nonformat_terminate = true;
        }, [](auto &&) { assert(false); }
        }, left.object);
        return left;
    }
    symbol_type __process_10(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](default_object_type&$$) {
            check_reset(ignore_duplicate_production) info.ignore_duplicate_production = true;
        }, [](auto &&) { assert(false); }
        }, left.object);
        return left;
    }
    symbol_type __process_11(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](default_object_type&$$) {
            check_reset(ignore_anonym_for_cr) info.ignore_anonym_for_cr = true;
        }, [](auto &&) { assert(false); }
        }, left.object);
        return left;
    }
    symbol_type __process_12(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](default_object_type&$$) {
            check_reset(property_list_for_auto_generate) info.property_list_for_auto_generate = true;
        }, [](auto &&) { assert(false); }
        }, left.object);
        return left;
    }
    symbol_type __process_13(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](default_object_type&$$) {
            check_reset(ignore_batch_for_anonym) info.ignore_batch_for_anonym = true;
        }, [](auto &&) { assert(false); }
        }, left.object);
        return left;
    }
    symbol_type __process_14(const ll left_type, std::vector<symbol_type> &right, pass_info &info) {
        symbol_type left = { left_type, (default_object_type){} };
        std::visit(overloaded{[&info](default_object_type&$$) {
            check_reset(show_all_collisions) info.show_all_collisions = true;
        }, [](auto &&) { assert(false); }
        }, left.object);
        return left;
    }
    namespace utils {
        const std::string raw_type_map[] = {
            "$start$",
            "$__ann_3",
            "ana_item",
            "analysis_program",
            "path",
            "int",
            "-no_parse_range_warn",
            "-restrict_nonformat_terminate",
            "-ignore_duplicate_production",
            "-ignore_anonym_for_cr",
            "-property_list_for_auto_generate",
            "-ignore_batch_for_anonym",
            "-show_all_collisions",
            "-o",
            "-l",
            "-thread",
            "$eof$",
        };
        const std::string * const type_name_map = raw_type_map + 4;
    }
    constexpr symbol_type (*semantic_list[])(const ll , std::vector<symbol_type> &, pass_info &) = {
        __default_object_type_semantic_process_,
        __default_object_type_semantic_process_,
        __left_combine_auto_generated_acproduction_,
        __initial_auto_generated_acproduction_,
        __process_4,
        __process_5,
        __process_6,
        __process_7,
        __process_8,
        __process_9,
        __process_10,
        __process_11,
        __process_12,
        __process_13,
        __process_14,
    };
    constexpr ll eof = 12;
    constexpr ll left_map[] = {
        -4,
        -1,
        -3,
        -3,
        -2,
        -2,
        -2,
        -2,
        -2,
        -2,
        -2,
        -2,
        -2,
        -2,
        -2,
    };
    constexpr size_t pop_size_map[] = {
        2,
        1,
        2,
        1,
        1,
        2,
        2,
        2,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
    };
    constexpr ll action_table[21][13] = {
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        {2, 0, 5, 15, 6, 7, 13, 8, 16, 3, 9, 11, 0, },
        {-4, 0, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, },
        {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        {-5, 0, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, },
        {-8, 0, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, },
        {-10, 0, -10, -10, -10, -10, -10, -10, -10, -10, -10, -10, -10, },
        {-11, 0, -11, -11, -11, -11, -11, -11, -11, -11, -11, -11, -11, },
        {-13, 0, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, },
        {10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        {-6, 0, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, },
        {0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        {-7, 0, -7, -7, -7, -7, -7, -7, -7, -7, -7, -7, -7, },
        {-12, 0, -12, -12, -12, -12, -12, -12, -12, -12, -12, -12, -12, },
        {2, 0, 5, 15, 6, 7, 13, 8, 16, 3, 9, 11, -1, },
        {-9, 0, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {-14, 0, -14, -14, -14, -14, -14, -14, -14, -14, -14, -14, -14, },
        {-2, 0, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19, },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        {-3, 0, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, },
    };
    constexpr ll goto_table[15][4] = {
        {0, 0, 0, 0, },
        {18, 20, 14, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 0, 0, 0, },
        {0, 17, 0, 0, },
    };
#ifdef TEST
    struct Logger {
        ostream &os = std::cout;
        template <typename ...Args>
        Logger &operator()(Args ...msgs) {
            (os << ... << msgs) << endl;
            return *this;
        }
    };
#endif
    
    using std::pair;
    class SyntacticAnalyzer {
#ifdef TEST
        Logger logger;
#endif
        // the analyze stacks
        stack<pair<size_t, symbol_type>> astack;
        pass_info &info;
        
    public:
        void init() {
            astack.emplace(1, symbol_type{
                utils::raw_type_map - utils::type_name_map, default_object_type{}
            });
        }
#ifdef TEST
        SyntacticAnalyzer(ostream &os, pass_info &info): logger{os}, info(info) {
            init();
        }
#endif
        SyntacticAnalyzer(pass_info &info): info(info)
#ifdef TEST
        , logger{cout}
#endif
        {
            init();
        }
        SyntacticAnalyzer &operator()(const token_type *token) {
            analyze(token);
            return *this;
        }
        void analyze(const token_type *token_ptr) {
            ll sym_type = token_ptr ? ll(get_type(*token_ptr)) : eof;
#ifdef TEST
            logger("Condition <", astack.top().first, "> Received '", utils::type_name_map[sym_type], "':");
#endif
            ll next_action = action_table[astack.top().first][sym_type];
            while (next_action < 0) { // do all reduce
                const size_t POP_SIZE = pop_size_map[-next_action];
                vector<symbol_type> right(POP_SIZE);
                for (size_t i = 0; i < POP_SIZE; ++i) {
                    right[POP_SIZE - 1 - i] = std::move(astack.top().second);
                    astack.pop();
                }
                ll left_type = left_map[-next_action];
                // to access goto table, "-symbol - 1"
                size_t condition = goto_table[astack.top().first][-left_type - 1];
                if (!condition) throw "This should not happen";
                astack.emplace(condition, semantic_list[-next_action](left_type, right, info));
                next_action = action_table[astack.top().first][sym_type];
#ifdef TEST
                logger("REDUCE to symbol: ", utils::type_name_map[left_type],
                       ", Condition <", condition, '>');
#endif
            } // end reduce
            if (!next_action)
                throw 1;
            astack.emplace(next_action, token_ptr ? symbol_type{sym_type, *token_ptr} : symbol_type{});
#ifdef TEST
            logger("Accept '", utils::type_name_map[sym_type], "' SHIFT to ", next_action)
            ("=========================================================================")
            ();
#endif
        }
    };
    
    template <typename T, typename ...InitArgs>
    pass_info analyze(const T &token_stream,
#ifdef TEST
                      ostream &os = cout,
#endif
                      InitArgs ...args) {
        pass_info r(std::forward<InitArgs>(args)...);
        SyntacticAnalyzer analyzer(
#ifdef TEST
                                   os,
#endif
                                   r);
        for (auto &token: token_stream) {
            analyzer(&token);
        }
        analyzer(nullptr);
        return r;
    }
    
}

using std::cout;
using std::endl;

struct Logger {
    ostream &os = cout;
    template <typename ...Args>
    Logger &operator()(Args ...msgs) {
        (cout << ... << msgs) << endl;
        return *this;
    }
};

Logger logger;

void print_usage() {
    logger("usage:")
    ("rtsl [property list] <source file path> [property list]")
    ("properties:")
    ("-o <output file path> *:default is the same as source file with postfix .cpp")
    ("-thread <unsigned int to specify thread amount> *: [1 - 100]")
    ("-no_parse_range_warn")
    ("-restrict_nonformat_terminate")
    ("-ignore_duplicate_production")
    ("-ignore_anonym_for_cr")
    ("-property_list_for_auto_generate")
    ("-ignore_batch_for_anonym")
    ("-show_all_collisions");
}

#include <fstream>
#include <sstream>
#include "except/TranslateException.h"

using std::ofstream;
using std::ifstream;
using std::stringstream;

int run(int argc, const char *argv[]) {
    if (argc == 1) { // print usage
        print_usage();
    }
    CommandParseObject raw_cmd;
    using namespace runner;
    SyntacticAnalyzer analyze(raw_cmd);
    for (int i = 1; i < argc; ++i) {
        string s(argv[i]);
        try {
            analyze(&s);
        } catch (const CommandParseException &e) {
            cout << e << endl;
            return 1;
        }
    }
    try {
        analyze(nullptr);
    } catch (const CommandParseException &e) {
        cout << e << endl;
        return 1;
    }
    
    using namespace rules_translator;
    CommandObject cmd;
    cmd.allow_nonformat_terminate = !raw_cmd.restrict_nonformat_terminate;
    cmd.do_batch_for_anonym = !raw_cmd.ignore_batch_for_anonym;
    cmd.ignore_duplicate_production = raw_cmd.ignore_duplicate_production;
    cmd.include_auto_generate_for_conflict_resolving
        = !raw_cmd.ignore_anonym_for_cr;
    cmd.no_parse_range_warn = raw_cmd.no_parse_range_warn;
    cmd.property_list_for_auto_generate = raw_cmd.property_list_for_auto_generate;
    cmd.show_all_collisions = raw_cmd.show_all_collisions;
    
    auto &source_path = raw_cmd.tsl_file_path;
    if (source_path.empty() ||
        source_path.size() <= 4 ||
        source_path.substr(source_path. size() - 4, 4) != ".tsl") {
        logger("Must specify source file position");
        return 2;
    }
    ifstream source(source_path);
    if (!source.is_open()) {
        logger("Cannot open source file.");
        return 3;
    }
    stringstream ss;
    char *buffer = new char[1024];
    do {
        source.read(buffer, 1024);
        ss.write(buffer, source.gcount());
    } while (source.gcount() == 1024);
    delete []buffer;
    ofstream tar(raw_cmd.target_file_path ?
                 *raw_cmd.target_file_path :
                 source_path.substr(0, source_path.size() - 3).append("cpp"));
    if (!tar.is_open()) {
        logger("Cannot open target file.");
        return 4;
    }
    ofstream log_path(raw_cmd.log_path ?
                      *raw_cmd.log_path :
                      "");
    if (raw_cmd.log_path && !log_path.is_open()) {
        logger("Cannot open log file.");
        return 5;
    }
    ::utils::Logger log(raw_cmd.log_path ? log_path : cout,
                        raw_cmd.log_path ? log_path : cout,
                        raw_cmd.log_path ? log_path : cout);
    ::utils::PoolManager pool(raw_cmd.thread_amount ? *raw_cmd.thread_amount - 1 : 1, cmd.stop);
    FileInteractor fi(ss, tar, log, cmd);
    try {
        auto info = parse(fi, cmd, log);
        generateTable(*info, fi, cmd, log, pool);
        generateAnalyzer(fi, cmd, log);
    } catch (const except::TranslateException &e) {
        cout << e.message << endl;
        return 1;
    }
    
    return 0;
}

