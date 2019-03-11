//
//  TableGenerator.hpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef TableGenerator_hpp
#define TableGenerator_hpp

#include "../include/core.h"
#include "../utils/semaphore.hpp"
#include <unordered_map>
#include <unordered_set>
#include <forward_list>
#include <type_traits>
#include <set>
#include <iomanip>


#ifdef TEST
#include <iostream>
using std::cout;
using std::endl;
#endif

using std::set;
using std::pair;
using std::unordered_map;
using std::shared_ptr;
using std::unordered_set;
using std::forward_list;

namespace std {
    using rules_translator::ProductionWithDoc;
    template <>
    struct hash<ProductionWithDoc> {
        size_t operator()(const ProductionWithDoc &pwd) const {
            return size_t(&(*pwd.doc));
        }
    };
}

namespace rules_translator {
    using condition_package = unordered_map<ProductionWithDoc, unordered_set<symbol>>;
    
    using pid = size_t;
}

namespace std {
    using rules_translator::condition_package;
    template <>
    struct hash<pair<size_t, condition_package>> {
        size_t operator()(const pair<size_t, condition_package> &p) const {
            return p.first;
        }
    };
}

namespace rules_translator::table_generator {
    
    using namespace utils;
    
    
    // this pass info is the inner data structure within table genertating phrase
    struct pass_info {
        ParseInfo &pi;
        CommandObject &cmd;
        Logger &logger;
        PoolManager &pool;
        
        ActionTable &action_table;          // table[line][symbol]
        mutex ata_mutex;                    // action table mutex
        GotoTable &goto_table;              // table[line][symbol]
        
        // the array of a forward_list
        forward_list<const Production *> *left_map; // the left set of Productions, for it should always be traverse, a forward list is better
        // as every nonterminate has a first_set, an array is enough
        unordered_set<symbol> *first_set;
        // if with err, the function should not output, but just return
        bool err = false;
#define log_generator(func_name) \
struct rep##func_name {\
Logger &logger;\
template <typename ...Args>\
rep##func_name &operator()(Args &&...msgs) {\
logger.func_name(std::forward<Args>(msgs) ...);\
return *this;\
}\
template <typename ...Args>\
void end(Args &&...msgs) {\
logger.func_name##ln(std::forward<Args>(msgs) ...);\
}\
};\
rep##func_name log_func##func_name() {\
return rep##func_name{logger};\
}
        log_generator(log)
        log_generator(err)
        log_generator(warn)
        
#undef log_generator
        
        template <typename T>
        auto print_symbol(symbol s, T &&log_func) {
            if (s >= 0) return log_func(std::quoted(pi.type_map[s], '\''), " ");
            else return log_func(pi.type_map[s], " ");
        };
        template <typename T>
        auto print_production(const Production &p, T &&log_func) {
            log_func("pid <", p.pid, ">: \n");
            print_symbol(p.left, log_func)(":= ");
            for (auto s: p.right)
                print_symbol(s, log_func);
            log_func.end();
        };
        template <typename T>
        auto print_package(const condition_package &package, T &&log_func) {
            for (auto &pp: package) {
                print_symbol(pp.first.p.left, log_func)(":= ");
                for (auto it = pp.first.p.right.begin(); it != pp.first.p.right.end(); ++it) {
                    if (it == pp.first.doc) log_func("(^)");
                    print_symbol(*it, log_func);
                }
                if (pp.first.end()) log_func("(^)");
                log_func(", { ");
                for (auto s: pp.second)
                    log_func(pi.type_map[s])(", ");
                log_func.end("}");
            }
        };
        
        // to calculate the priority
        // return if p1s win
        bool tackle_sr_conflict(set<size_t> p1s, size_t p2) {
            optional<bool> win_type;
            size_t temp = 0;
            // find solution
            for (auto p1: p1s) {
                // elong the life time of return value
                const auto &k = pi.crtable.resolve(p1, p2, conflict_type::shift_reduce);
                if (!k) continue;
                if (!win_type){
                    win_type = k;
                    temp = p1;
                }
                else if (*win_type != *k) {
                    // here is a specifying error
                    auto &&logr = log_funcerr();
                    logr("Conflict Resolving Conflict happened: \n")
                    ("they're (rank from min priority to max): \n");
                    if (*win_type) std::swap(temp, p1); // to let temp out in min
                    print_production(pi.ps[temp], logr);
                    print_production(pi.ps[p2], logr);
                    print_production(pi.ps[p1], logr);
                    collision_tackle_func();
                    return false;
                }
            }
            if (!win_type) { // check whether there are some solution
                auto &&logr = log_funcerr();
                logr("No Conflict Resoving Option has specified: \n");
                print_production(pi.ps[p2], logr);
                for (auto p1: p1s)
                    print_production(pi.ps[p1], logr);
                collision_tackle_func();
                return false;
            }
            if (!*win_type) return false; // means nothing to do, because the original value has higher priority
            return true;
        };
        
        void collision_tackle_func() {
            if (cmd.show_all_collisions) err = true;
            else {
                cmd.stop = true;
                throw except::ConflictResolveCollisionTranslateException();
            }
        }
        
        
    private:
        unordered_map<size_t,
            unordered_map<size_t,
                unordered_map<pair<size_t, condition_package>, size_t>>> package_map;
        size_t next_condition = 1;
        mutex package_map_mutex;
        
        size_t *nullable_bitmap;
    public:
        
        pass_info(ParseInfo &info,
                  CommandObject &cmd,
                  utils::Logger &logger,
                  utils::PoolManager &pool,
                  ActionTable &acta,
                  GotoTable &gota):
        pi(info), cmd(cmd), logger(logger), pool(pool),
        action_table(acta), goto_table(gota){
            left_map = new forward_list<const Production *>[-info.start];
            left_map -= info.start; // info.start is minus, to point to the "0" position
            first_set = new unordered_set<symbol>[-info.start];
            first_set -= info.start;
            nullable_bitmap = new size_t[-info.start / sizeof(size_t) + bool(-info.start % sizeof(size_t))]();
        }
        ~pass_info() {
            left_map += pi.start;
            delete []left_map;
            first_set += pi.start;
            delete []first_set;
            delete []nullable_bitmap;
        }
        
        bool set_nullable(symbol s) { // return whether it has already set
#ifdef DEVP
            if (s >= 0 || s < pi.start)
                throw except::OutOfRangeInnerException("This symbol is out of range to set nullable");
#endif
            s = -s - 1;
            bool r = nullable_bitmap[s / sizeof(size_t)] & (1 << (s % sizeof(size_t)));
            nullable_bitmap[s / sizeof(size_t)] |= (1 << (s % sizeof(size_t)));
            return r;
        }
        
        bool is_nullable(symbol s) {
            if (s >= 0) return false;
#ifdef DEVP
            if (s < pi.start)
                throw except::OutOfRangeInnerException("This symbol is out of range to set nullable");
#endif
            s = -s - 1;
            return nullable_bitmap[s / sizeof(size_t)] & (1 << (s % sizeof(size_t)));
        }
        /**
         * About implementation:
         *      This the data structure is 3-level map, the mission of the hash code of a condition_package is divided
         *      into these three parts, and the last one is pair<size_t, condition_package>, the last one
         *      is the final hash code part of condition_package, so, hash<pair<size_t, condition_package>> == p.first
         *
         * Returns:
         *      pair<condition number, insert succeed>
         */
        pair<size_t, bool> get_condition(const condition_package &package) {
            size_t a = package.size(), b = 0, c = 0;
            for (auto &p: package) {
                a += p.first.p.right.size();
                a += p.second.size();
                
                size_t m = ll(p.first.p.pid) - p.first.p.left;
                size_t k = 0, n = 0;
                static constexpr size_t range = (sizeof(size_t) / 8);
                static constexpr size_t upper_bound = size_t(~0) << (1 - range);
                for (auto s: p.second) {
                    k = (k << 4) + s;
                    n = (n << 4) + m ^ s;
                    if (k & upper_bound) k = k ^ (-s) >> (2 * range);
                    if (n & upper_bound) n = n ^ (m ^ (-s)) >> (2 * range);
                }
                n ^= (m | (m - 1));
                c += n >> (2 * range);
                if (c & upper_bound) c = c ^ m >> (2 * range);
                k += size_t(&(*p.first.doc)) >> 24;
                b = b ^ k + k;
            }
            // here, acquire the lock, and automatically release it when leave
            std::lock_guard<mutex> lock_g(package_map_mutex);
            auto &cd = package_map[a][b][std::make_pair(c, std::ref(package))];
            if (!cd) {
                cd = next_condition++;
                return std::make_pair(cd, true);
            }
            else return std::make_pair(cd, false);
        }
    };
    
}

#endif /* TableGenerator_hpp */
