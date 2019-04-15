//
//  TableGenerator.cpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#include "TableGenerator.hpp"

namespace rules_translator {
    
    using table_generator::pass_info;
    using namespace utils;
    
    std::pair<std::shared_ptr<ActionTable>, std::shared_ptr<GotoTable>>
    generateTable(ParseInfo &pi,
                  FileInteractor &fi,
                  CommandObject &cmd,
                  utils::Logger &logger,
                  utils::PoolManager &pool) {
        // the eof symbol is also inside it, because eof can be used to shift or reduce
        auto ptr_action_table = shared_ptr<ActionTable>(new ActionTable(pi.eof + 1));
        // the start symbol itself is not inside it
        auto ptr_goto_table = shared_ptr<GotoTable>(new GotoTable(-pi.start));
        pass_info info {
            pi,
            cmd,
            logger,
            pool,
            *ptr_action_table,
            *ptr_goto_table
        };
        
#pragma message("Preprocess Phrase")

        /* The Preprocess Phrase */
        // to firstly detach to run the function of collecting left
        // then this one to run to collect nullable and first set
        {
            // to wait for left to be finished
            semaphore left_semaphore;
            pool.execute([&left_semaphore, &info]() {
                for (auto &p: info.pi.ps)   // because it's just a list, no need to cache pointer
                    info.left_map[p.left].push_front(&p);
                left_semaphore.signal();
            });
            
            // nullable analysis
            {
                int change;
                do {
                    change = 0;
                    for (auto &p: pi.ps) {
                        for (auto &s: p.right)
                            if (!info.is_nullable(s)) goto brk;
                        change += info.set_nullable(p.left);
                    brk:
                        ;
                    }
                } while (change > 0);
            }
            
            // first analysis
            {
                int change;
                do {
                    change = 0;
                    for (auto &p: pi.ps)
                        for (auto &s: p.right)
                            if (s >= 0) {
                                change += info.first_set[p.left].insert(s).second;
                                break;
                            }
                            else {
                                const size_t OS = info.first_set[p.left].size();
                                info.first_set[p.left].insert
                                    (info.first_set[s].begin(), info.first_set[s].end());
                                change += (OS != info.first_set[p.left].size());
                                if (!info.is_nullable(s)) break;
                            }
                } while (change > 0);
#ifdef TEST_TABLEGEN
                logger.logln("Finish first analysis, Result:");
                auto &&logr = info.log_funclog();
                logr.end("Left Map: ");
                for (ll s = pi.start; s < 0; ++s) {
                    logr.end(info.pi.type_map[s], ": ");
                    for (auto &p: info.left_map[s])
                        info.print_production(*p, info.log_funclog());
                    logr.end();
                }
                logr("First Map:").end();
                for (ll s = pi.start; s < 0; ++s) {
                    logr.end(info.pi.type_map[s], ": ");
                    for (auto &ts: info.first_set[s])
                        logr(info.pi.type_map[ts], ", ");
                    logr.end();
                }
#endif
            }
            
            // to join
            left_semaphore.wait();
        }
        
#pragma message("Define calculate_condition")
        
        
#ifdef TEST_TABLEGEN
        
#endif
        
std::function<void (condition_package, size_t, symbol, semaphore &, set<pid> &)>
calculate_condition = [&info, &pi, &pool, &logger, &calculate_condition
#ifdef TEST_TABLEGEN
                             // capture symbols when it's test
#endif
                            ]
        // package is the raw_package to fill
        // p1s is the saved ids
(condition_package package, size_t previous_condition, symbol path_symbol, semaphore &sem, set<pid> &p1s) {
#ifdef TEST_TABLEGEN
    logger.log("From Condition <").log(std::to_string(previous_condition)).logln("> get package:");
    info.print_package(package, info.log_funclog());
    logger.logln("-------------------------------------------------------------------------------").logln();
#endif
    // trace the package
    {
        int change;
        auto put_set = [&change, &package, &info](symbol s, unordered_set<symbol> &to_put) {
            if (to_put.empty()) return;
            for (auto &pp: info.left_map[s]) {
                auto &set = package[ProductionWithDoc(*pp)];
                const size_t OS = set.size();
                set.insert(to_put.begin(), to_put.end());
                change += (OS != set.size());
            }
        };
        auto insert_first = [&info](symbol s, unordered_set<symbol> &set) {
            if (s >= 0) set.insert(s);
            else set.insert(info.first_set[s].begin(), info.first_set[s].end());
        };
        do {
            change = 0;
            for (auto &item: package) {
                // no next, wait for reduce
                auto &pwd = item.first;
                if (pwd.end()) continue;
                symbol s = pwd.getNextSymbol();
                if (s >= 0) continue;
                
                if (pwd.last()) {
                    put_set(s, item.second);
                    continue;
                }
                
                const auto END = pwd.end_doc();
                auto dit = pwd.getFollowingSymbols();
                unordered_set<symbol> temp_set;
                do {
                    insert_first(*dit, temp_set);
                    if (!info.is_nullable(*dit)) goto cont;
                    ++dit;
                } while (dit != END);
                temp_set.insert(item.second.begin(), item.second.end());
            cont:
                put_set(s, temp_set);
            }
        } while (change > 0);
    } // end trace package block
#ifdef TEST_TABLEGEN
    logger.logln("After tracing, got package:");
    info.print_package(package, info.log_funclog());
    logger.logln("===============================================================================").logln();
#endif
    
    // the list for sems
    forward_list<semaphore> sems;
    unordered_set<ProductionWithDoc> ptrs;
    semaphore p_sem;
    // next path_symbol and its pids
    unordered_map<symbol, set<pid>> tsp1s; // the symbol-p1 sets
    
    if (info.cmd.stop) return;
    
    // Start Analysis
    auto con_r = info.get_condition(package);
    size_t condition = con_r.first;
    if (path_symbol >= 0) {
        // keep the original one, if the value it is, rewrite it
        // or, update the original one, then calculate priority
        // if less, go out right away,
        // if more, wait for another round
        // if nothing specified or collision occurs, throw it
        ll keep = 0; // keep the original one
        while (1) {
            bool mark = false; // to mark if keep updated
            info.ata_mutex.lock();
            auto &ov = info.action_table[previous_condition][path_symbol];
            if (ov == keep) ov = con_r.first; // this is more likely to happen
            else {
                keep = ov; // this must be reduce-shift collision
                mark = true;
            }
            info.ata_mutex.unlock();
            if (!mark) {
#ifdef TEST_TABLEGEN
                if (keep)
                    logger.logln("Reset ActionTable[", previous_condition, "][",
                           std::quoted(pi.type_map[path_symbol], '\''),
                           "] to: SHIFT <", previous_condition, "> to <", condition, ">.");
                else
                    logger.logln("SHIFT <", previous_condition, "> by '", pi.type_map[path_symbol], "' to <",
                                 condition, ">");
#endif
                break;
            }
#ifdef TEST_TABLEGEN
            logger.logln("Already existed REDUCE by symbol '", pi.type_map[path_symbol], "': ");
            info.print_production(pi.ps[-keep], info.log_funclog());
            logger.logln("Try to set it to: SHIFT to <", condition, ">");
#endif
            
            if (info.tackle_sr_conflict(p1s, -ov, sem)) {
#ifdef TEST_TABLEGEN
                logger.logln("Set SHIFT fail in: from condition <", previous_condition, "> to condition <",
                           condition, ">.");
#endif
                break;
            }
        }
    }
    else { // for goto table will never has collision
#ifdef TEST_TABLEGEN
        logger.logln
        ("SHIFT <", previous_condition, "> by ", info.pi.type_map[path_symbol], " to <", condition, ">.");
#endif
        info.goto_table[previous_condition][path_symbol] = con_r.first;
    }
    if (!con_r.second) {
#ifdef TEST_TABLEGEN
        logger.logln("This condition already existed.");
#endif
        goto ret;
    }
    
    
    p_sem.signal(); // to ensure the first time to access ptrs
    // to tackle condition
    for (auto &pp: package) {
        p_sem.wait(); // wait for OK to access ptrs
        if (!ptrs.insert(pp.first).second) {
            p_sem.signal(); // OK for next time to visit
            continue;
        }
        sems.emplace_front();
        auto &nsem = sems.front();
pool.execute
([&info, &tsp1s, &nsem, &pp, &package, &ptrs, condition, &calculate_condition, &p_sem]()->void {
    if (pp.first.end()) { // reduce
        p_sem.signal(); // ok to access ptrs
        ll to_set = -intptr_t(pp.first.p.pid);
        for (auto s: pp.second) {
            ll keep = 0;
            while (1) {
                bool mark = false;
                info.ata_mutex.lock();
                auto &ov = info.action_table[condition][s];
                if (ov == keep) ov = to_set;
                else {
                    mark = true;
                    keep = ov;
                }
                info.ata_mutex.unlock();
                if (!mark) {
#ifdef TEST_TABLEGEN
                    if (!keep)
                        info.logger.logln("Condition<", condition, ">, REDUCE Production by '",
                                          info.pi.type_map[s], "': ");
                    else
                        info.logger.logln("Reset Action[", condition, "]['", info.pi.type_map[s],
                                        "'] to REDUCE:");
                    info.print_production(info.pi.ps[pp.first.p.pid], info.log_funclog());
#endif
                    break;
                }
                
#ifdef TEST_TABLEGEN
                info.logger.log("Already existed ",
                                  (keep > 0 ? "SHIFT" : "REDUCE"),
                                  " by symbol");
                info.print_symbol(s, info.log_funclog())(":\n");
                if (keep < 0)
                    info.print_production(info.pi.ps[-keep], info.log_funclog());
                else info.logger.logln("<", keep, ">");
                info.logger.logln("Try to set it to: REDUCE to:");
                info.print_production(pp.first.p, info.log_funclog());
#endif
                
                // calculate priority
                optional<bool> win_type;
                if (keep < 0) { // reduce-reduce type
                    win_type = info.pi.crtable.resolve(-to_set, -keep, conflict_type::reduce_reduce);
                    if (!win_type) {
                        auto &&logr = info.log_funcerr();
                        logr("No Conflict Resolving option has been set:\n");
                        info.print_production(info.pi.ps[-to_set], info.log_funcerr());
                        info.print_production(info.pi.ps[-keep], info.log_funcerr());
                        info.collision_tackle_func(nsem);
                        break;
                    }
                    if (!*win_type) break;
                } // end reduce-reduce conflict resoving
                else { // shift-reduce conflict resoving
                    
                    // tsp1s must already be set
                    if (!info.tackle_sr_conflict(tsp1s[s], -to_set, nsem)) break;
                    
                } // end shift-reduce conflict resoving
            }
        }
        nsem.signal();
        return;
    }
    
    // shift
    auto next_path_symbol = pp.first.getNextSymbol();
    condition_package next_package;
    auto &next_p1s = tsp1s[next_path_symbol];
    for (auto &npp: package)
        if (!npp.first.end() && npp.first.getNextSymbol() == next_path_symbol) {
            ptrs.insert(npp.first);
            next_package[npp.first.next()] = std::move(npp.second);
            next_p1s.insert(npp.first.p.pid);
        }
    p_sem.signal(); // ok to access ptrs
    if (info.cmd.stop) return;
    calculate_condition(std::move(next_package), condition, next_path_symbol, nsem, next_p1s);
});
    }
    
    
    
    for (auto &tsem: sems)
        tsem.wait();
ret:
    sem.signal();
}; // end calculate_condition
        
#pragma message("Start Analysis")
        
        /* Start Analysis */
        
        { // prepare for this
            condition_package package;
            package[ProductionWithDoc(pi.ps[0])];
            semaphore sem;
            set<pid> empty_set;
            calculate_condition(std::move(package), 0, 0, sem, empty_set);
            sem.wait();
        }
        
        if (info.err) throw except::StopMark{};
        
        // write table
        auto write_table = [&fi](const string_view &name, CounselTable &table) {
            const size_t LINE_AMOUNT = table.lineAmount();
            const size_t COLUMN_AMOUNT = table.columnAmount();
            fi.writeln("constexpr ll ", name, "[", LINE_AMOUNT, "][", COLUMN_AMOUNT, "] = {");
            for (size_t i = 0; i < LINE_AMOUNT; ++i) {
                fi.write("{");
                ll *ptr = table[i];
                for (size_t j = 0; j < COLUMN_AMOUNT; ++j)
                    fi.write(ptr[j], ", ");
                fi.writeln("},");
            }
            fi.writeln("};");
        };
        write_table("action_table", ptr_action_table->getTable());
        write_table("goto_table", ptr_goto_table->getTable());
        
        return std::make_pair(ptr_action_table, ptr_goto_table);
    }
    
}
