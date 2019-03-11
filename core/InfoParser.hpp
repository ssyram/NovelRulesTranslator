//
//  InfoParser.hpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef InfoParser_hpp
#define InfoParser_hpp

/* This file should only be included by InfoParser.cpp */
/* It's only for coding convienice */
/* essentially, it's a part of InfoParser.cpp */

#include "../include/FileInteractor.h"
#include "../include/core.h"
#include "fi_info/fi_type.h"
#include "fi_info/fi_funcs.hpp"
#include "../utils/funcs.hpp"
#include "../utils/template_funcs.h"
#include <variant>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iomanip>
#include <optional>
#include <queue>
#include <stack>
#include <string_view>
#include "../include/GlobalInfo.h"

using std::string_view;
using std::list;
using std::stack;
using std::queue;
using std::optional;
using std::nullopt;
using std::variant;
using std::visit;
using std::shared_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::pair;
using std::make_pair;
using std::quoted;
using std::ostream;
using std::endl;

namespace rules_translator {
    struct rank_vec;
    
    struct e_production {
        const Production &p;
        e_production(const Production &p): p(p) { }
        bool operator==(const e_production &ep) const {
            if (p.left != ep.p.left || p.right.size() != ep.p.right.size())
                return false;
            for (size_t i = 0; i < p.right.size(); ++i)
                if (p.right[i] != ep.p.right[i]) return false;
            return true;
        }
    };
}

using ll = long long;

namespace std {
    template <>
    struct hash<rules_translator::e_production> {
        size_t operator()(const rules_translator::e_production &ep) const {
            // algorithm: left put its low 7 to the most left 7
            // the right put unsigned << 3
            size_t r = 0, x = 0;
            
            for (size_t s: ep.p.right) {
                r = (r << 3) + (s & (~0 >> 16));
                if ((x = r & (size_t(1) << 60)) != 0) {
                    r ^= (x >> 60);
                    r &= ~x;
                }
            }
            r = utils::set_pos(r, 0, 57);
            r |= utils::set_pos(-ep.p.left, 57, 7);
            return r;
        }
    };
    template <>
    struct hash<pair<shared_ptr<rules_translator::rank_vec>, ll>> {
        size_t operator()(const pair<shared_ptr<rules_translator::rank_vec>, ll> &p) const {
            return size_t(&(*(p.first))) + p.second;
        }
    };
}

namespace rules_translator {
    using namespace utils;
    
    template <typename ...Ts> struct overloaded: Ts... { using Ts::operator() ...; };
    template <typename ...Ts> overloaded(Ts...) -> overloaded<Ts...>;
    
    using std::pair;
    
    extern const char *INIT_AGACP;
    extern const char *LEFT_COMBINE_AGACP;
    extern const char *RIGHT_COMBINE_AGACP;
    
    extern shared_ptr<string> DEFAULT_OBJECT_TYPE_PROCESS_NAME;
    
    struct pass_info;
    struct iter_mark;
    using raw_sym = variant<symbol, iter_mark>;
    
    class iter_mark {
        // the first can_null is the type of this block, '[]' or '{}'
        // the second one is whether it really can be null
        bool can_null = false, real_can_null = false;
        
        struct analyze_info {
            list<vector<raw_sym>> &inss;
            bool can_null;
            list<vector<raw_sym>>::iterator it;
            vector<symbol> temp_right;
            stack<pair<size_t, size_t>> working_stack; // from where to read, from where to insert
            analyze_info(list<vector<raw_sym>> &inss, bool can_null): inss(inss), it(inss.end()), can_null(can_null) { }
            void fill_temp_right(size_t sp) { // start pos
                auto &r = *it;
                const size_t SIZE = r.size();
                for (size_t i = sp; i < SIZE; ++i) {
                    auto &s = r[i];
                    if (s.index()) {
                        auto &im = std::get<iter_mark>(s);
                        size_t pos = temp_right.size();
                        if (im.modify(temp_right)) working_stack.emplace(i, pos);
                    }
                    else temp_right.push_back(std::get<symbol>(s));
                }
            }
            bool analyze(vector<symbol> &pr) {
                if (working_stack.empty()) {
                    if (it == inss.end()) it = inss.begin();
                    else {
                        ++it;
                        if (it == inss.end()) return false; // this means it can_null, so, return null
                    }
                    
                    temp_right.clear();
                    fill_temp_right(0);
                }
                else {
                    auto top = working_stack.top().first;
                    temp_right.erase(temp_right.begin() + working_stack.top().second, temp_right.end());
                    working_stack.pop();
                    fill_temp_right(top);
                }
                
                pr.insert(pr.end(), temp_right.begin(), temp_right.end());
                {
                    auto k = it;
                    ++k;
                    if (working_stack.empty() && k == inss.end() && !can_null) {
                        ++it; // let it == inss.end()
                        return false;
                    }
                }
                return true;
            }
        };
        
        // to avoid heavy copy cost
        shared_ptr<analyze_info> info;
        
    public:
        // do not pass analyze_info
        iter_mark(const iter_mark &itm): can_null(itm.can_null), real_can_null(itm.can_null), inside_syms(itm.inside_syms) { }
        iter_mark(iter_mark &&itm): can_null(itm.can_null), real_can_null(itm.can_null), inside_syms(std::move(itm.inside_syms)) { }
        explicit iter_mark(bool can_null): can_null(can_null), real_can_null(can_null) { }
        
        list<vector<raw_sym>> inside_syms;
        
        void push_sym(raw_sym &&rs) {
            if (inside_syms.empty()) inside_syms.push_back(vector<raw_sym>(1, std::move(rs)));
            else inside_syms.back().push_back(std::move(rs));
        }
        void push_or() {
            if (inside_syms.empty() || inside_syms.back().empty()) {
                if (!real_can_null) real_can_null = true;
                return;
            }
            inside_syms.push_back(vector<raw_sym>());
        }
        bool quiry_can_null() { return can_null; }
        
        bool modify(vector<symbol> &pr) {
            if (!info) info = shared_ptr<analyze_info>(new analyze_info{inside_syms, real_can_null});
            
            return info->analyze(pr);
        }
    };
    
    // about how to resolve auto duplication:
    //      just put [__ann_[token_num]] to the first place or the last place, and iter by itself
    struct raw_ac_production {
        // this means whether to apply the batch functions ("back" and "front")
        // to anonym functions -- the generated functions, default is false
        // this should be specified by CommandObject
        bool is_anonym = false;
        symbol left;
        vector<raw_sym> right;
        queue<shared_ptr<raw_ac_production>> temp_q;
        // after a Production is found, call this function
        // to push the target Production in
        const std::function<size_t (const Production &, const shared_ptr<string> &)> push_production;
        
        const static shared_ptr<string> INIT_AGAC;
        const static shared_ptr<string> LEFT_AGAC;
        const static shared_ptr<string> RIGHT_AGAC;
        
        // analyze the functions and push them back
        size_t analyze() {
            stack<pair<size_t, size_t>> working_stack;
            Production p(left);
            auto &pr = p.right;
            size_t r = 0;
            const size_t SIZE = right.size();
            size_t mark = 0;
            auto fill_func = [&mark, SIZE, this, &p, &r, &pr, &working_stack]() {
                for (size_t i = mark; i < SIZE; ++i) {
                    auto &s = right[i];
                    if (s.index()) {
                        auto &im = std::get<iter_mark>(s);
                        size_t pos = pr.size(); // the next element to append
                        if (im.modify(pr)) working_stack.emplace(i, pos); // if has next, input
                    }
                    else pr.push_back(std::get<symbol>(s));
                }
                size_t temp;
                if (is_anonym) {
                    if (!pr.empty()) {
                        if (pr.front() == p.left) temp = push_production(p, LEFT_AGAC);
                        else if (pr.back() == p.left) temp = push_production(p, RIGHT_AGAC);
                        else temp = push_production(p, INIT_AGAC);
                    }
                    else temp = push_production(p, INIT_AGAC);
                }
                else temp = push_production(p, nullptr);
                
                if (temp) r = temp;
            };
            fill_func();
            while (!working_stack.empty()) {
                // find mark
                mark = working_stack.top().first;
                pr.erase(pr.begin() + working_stack.top().second, pr.end());
                working_stack.pop();
                
                fill_func();
            };
            while (!temp_q.empty()) {
                temp_q.front()->analyze();
                temp_q.pop();
            }
            return r;
        }
        
        raw_ac_production
        (const Production &p,
         const std::function<size_t (const Production &,
                                     const shared_ptr<string> &)> &push_production):
        left(p.left), push_production(push_production)
        {
            for (auto s: p.right)
                right.push_back(s);
        }
        
        raw_ac_production
        (const symbol left,
         const std::function<size_t (const Production &, const shared_ptr<string> &)> &push_production):
        left(left),
        push_production(push_production),
        is_anonym(true) { }
        
        // do not pass working_stack
        raw_ac_production(const raw_ac_production &rap):
        is_anonym(rap.is_anonym),
        left(rap.left),
        right(rap.right),
        temp_q(rap.temp_q),
        push_production(rap.push_production) { }
        
        raw_ac_production(raw_ac_production &&rap):
        is_anonym(rap.is_anonym),
        left(rap.left),
        right(std::move(rap.right)),
        temp_q(std::move(rap.temp_q)),
        push_production(rap.push_production) { }
    };
    
    // pass a function to print the productions and throw exception
    class ConflictResolveSettingTable {
        unordered_map<size_t, unordered_map<size_t, pair<uint8_t, uint8_t>>> data;
        pass_info &info;
        void generate_exception(size_t pid1, size_t pid2, conflict_type &type);
    public:
        ConflictResolveSettingTable(pass_info &info): info(info) { }
        ~ConflictResolveSettingTable() {
#ifdef TEST
            static auto print_rule = [](size_t p1, size_t p2, uint8_t type) {
                std::cout << p1 << ", " << p2 << " - " << conflict_type_name_list[type] << endl;
            };
            for (auto &p1: data)
                for (auto &p2: p1.second) {
                    uint8_t validate = p2.second.first >> 4;
                    for (size_t i = 1; i <= 4; i <<= 1) {
                        if (validate & i) {
                            if (p2.second.first & i) print_rule(p1.first, p2.first, i);
                            else print_rule(p2.first, p1.first, i < 4 ? i ^ 3 : i); // because there is a swap
                        }
                    }
                }
#endif
        }
        
        void combination_set(size_t pid1, size_t pid2, bool left) {
            if (left) { // respect REDUCE
                set(pid1, pid2, conflict_type::reduce_shift);
                set(pid2, pid1, conflict_type::reduce_shift);
            }
            else { // right: respect SHIFT
                set(pid1, pid2, conflict_type::shift_reduce);
                set(pid2, pid1, conflict_type::shift_reduce);
            }
        } // end function combination_set
        
        // respect pid1
        void priority_set(size_t pid1, size_t pid2) {
            if (pid1 == pid2) throw except::IDEqualInnerException();
            set<true, true>(pid1, pid2, conflict_type::shift_reduce);
            set<true, true>(pid1, pid2, conflict_type::reduce_shift);
            set<true, true>(pid1, pid2, conflict_type::reduce_reduce);
        }
        
        // respect pid1
        template <bool strong = false, bool cover = false>
        void set(size_t pid1, size_t pid2, conflict_type type) {
            if (pid1 == pid2 && type == conflict_type::reduce_reduce)
                throw except::InvalidPointTypeParseException("REDUCE-REDUCE should not happen to the same id.");
            bool swap = pid1 > pid2;
            if (uint8_t(type) < 4 && swap) type = conflict_type(uint8_t(type) ^ 3);
            auto &k = swap ? data[pid2][pid1] : data[pid1][pid2];
            uint8_t vpos = uint8_t(type) << 4;
            uint8_t tar = swap ? 0 : uint8_t(type);
            uint8_t toset = vpos | tar;
            if (k.first & vpos) { // already set
                if (k.first & tar) { // is the same
                    if constexpr (strong)
                        k.second |= uint8_t(type);
                    return;
                }
                // not the same
                if constexpr (strong) { // strong set
                    if constexpr (!cover)
                        if (k.second & uint8_t(type)) // it's strong
                            generate_exception(pid1, pid2, type);
                    // not strong, just set
                    k.first ^= uint8_t(type);
                    k.second |= uint8_t(type);
                }
                else { // not strong
                    if (k.second & uint8_t(type)) return;
                    // also not strong
                    if constexpr (!cover)
                        generate_exception(pid1, pid2, type);
                    k.first ^= uint8_t(type);
                }
            }
            else { // not set
                if (pid1 == pid2) {
                    k.first |= (((uint8_t(type) | 3) << 4) | uint8_t(type));
                }
                k.first |= toset;
                if constexpr (strong) k.second |= uint8_t(type);
            }
        }
        
        ConflictResolveCounselTable getCounselTable() {
            unordered_map<size_t, unordered_map<size_t, uint8_t>> d;
            for (auto &spid: data)
                for (auto &lpid: spid.second)
                    d[spid.first][lpid.first] = lpid.second.first;
            return ConflictResolveCounselTable(std::move(d));
        }
    }; // end class ConflictResolveSettingTable
    
    struct combination_set {
        bool is_left;
        unordered_set<size_t> data;
        combination_set(bool is_left, ConflictResolveSettingTable &st): is_left(is_left), stable(st) { }
        ~combination_set() {
            for (auto it = data.begin(); it != data.end(); ++it)
                for (auto dit = it; dit != data.end(); ++dit)
                    stable.combination_set(*it, *dit, is_left);
        }
    private:
        ConflictResolveSettingTable &stable;
    }; // end struct combination_set
    
    struct rank_vec {
        bool auto_increase = true;
        // pair<pid, rank>
        vector<pair<size_t, ll>> data;
        ll get_next_rank() {
            if (data.empty()) return 0;
            ll r = data.back().second;
            return auto_increase ? r + 1 : r;
        }
        rank_vec(ConflictResolveSettingTable &st): stable(st) { }
        ~rank_vec() {
            if (data.size() < 2) return;
            // sort to: ranking from high to low priority
            std::sort(data.begin(), data.end(), [](auto &&t1, auto &&t2) {
                return t1.second > t2.second;
            });
            ll temp_rank = data[0].second;
            vector<pair<size_t, ll>>::const_iterator temp_it = data.begin();
            
            // to check which is the next one
            auto check_rank = [&temp_rank, &temp_it, this]() {
                for (auto it = temp_it + 1; it != data.end(); ++it)
                    if (it->second != temp_rank) {
                        temp_it = it;
                        return;
                    }
                temp_it = data.end();
            };
            check_rank();
            for (auto it = data.begin(); temp_it != data.end(); ++it) {
                if (it->second != temp_rank) {
                    temp_rank = it->second;
                    check_rank();
                }
                for (auto dit = temp_it; dit != data.end(); ++dit)
                    stable.priority_set(it->first, dit->first);
            }
        }
    private:
        ConflictResolveSettingTable &stable;
    }; // end struct rank_vec
    
    struct property_list {
        enum class strong_combination_type {
            not_set,
            left,
            right
        };
        unordered_set<shared_ptr<combination_set>> csets;
        // rank_vec and corresponding rank
        unordered_set<pair<shared_ptr<rank_vec>, ll>> rvec_info;
        // in rank means can specify single rank
        bool nofront = false, noback = false;
        // the specified rank, valid only when inrank == true
        ll rank;
        strong_combination_type sctype = strong_combination_type::not_set;
        
    }; // end struct property_list
    
    struct block_info {
        const size_t level, pos;
        property_list plist;
        // back_id will be pointed to
        size_t front_id, back_id = -1;
        shared_ptr<rank_vec> this_rank;
        bool withfront = false, withback = false;
        shared_ptr<block_info> previous;
        
    private:
        string name;
        list<pair<Production, shared_ptr<string>>> &ps;
    public:
        string get_name() {
            if (!name.empty()) return name;
            name = ((string("__") += std::to_string(level) += "_") += std::to_string(pos));
            return name;
        }
        
        block_info
        (shared_ptr<block_info> previous,
         shared_ptr<rank_vec> this_rank,
         size_t level, size_t pos,
         list<pair<Production, shared_ptr<string>>> &ps
         ): level(level), pos(pos), front_id(ps.size()), previous(previous), this_rank(this_rank), ps(ps) { }
        ~block_info() noexcept(false) {
            back_id = ps.size() - 1;
            // because front_id is the previous ps.size()
            // so, if back_id < front_id
            // it means the block is empty
            if (back_id < front_id)
                throw except::EmptyBlockParseException(string("Empty block is not allowed: block ") += get_name());
        }
    }; // end struct block_info
    
    namespace named_types {
        struct production { size_t pid; };                          // id: 0
        struct nsymbol { ll sid; };                                 // id: 1
        struct block { size_t front_id, *back_id; };                // id: 2
        struct acproduction { size_t front_id, back_id; };          // id: 3
        struct combination { shared_ptr<combination_set> ptr; };    // id: 4
        struct rank { shared_ptr<rank_vec> ptr;};                   // id: 5
        struct mix_property {                                       // id: 6
            shared_ptr<combination_set> cptr;
            shared_ptr<rank_vec> rptr;
        };
    }
    using named_type = variant<named_types::production, named_types::nsymbol, named_types::block, named_types::acproduction, named_types::combination, named_types::rank, named_types::mix_property>;
    
    
    
    /* Pass Info */
    
    
    
    struct pass_info {
        FileInteractor &fi;
        CommandObject &cmd;
        Logger &logger;
        // map of terminate symbol and name
        // because terminate symbols are seperated from
        // any other kinds of symbols
        unordered_map<string, symbol> ter_sym_name_map;
        // map of nonterminate symbol and other property
        unordered_map<string, named_type> name_type_map;
        pair<string, shared_ptr<string>> *nontermiante_cpp_type_map; // map[symbol]
        ll cpp_type_map_min_symbol; // the length of the type before, also, the min symbol type
        unordered_set<symbol> annonym_syms;
        list<shared_ptr<block_info>> block_stack;
        list<pair<Production, shared_ptr<string>>> ps;
        bool called_get_sym_list = false;
        
        ConflictResolveSettingTable crtable;
        
        ~pass_info() {
        }
        
    private:
        symbol next_nonterminate = -1;
        vector<size_t> block_name_mark;
        unordered_set<e_production> production_distinct_set;
        void inner_insert_pid(property_list &pl, std::function<void (combination_set &)> cinsert, std::function<void (rank_vec &, ll)> rinsert) {
            auto ins_pl = [this, &cinsert, &rinsert](property_list &pl) {
                for (auto &s: pl.csets)
                    cinsert(*s);
                for (auto &p: pl.rvec_info)
                    rinsert(*p.first, p.second);
            };
            ins_pl(pl);
            property_list *plptr = &pl;
            for (auto cur_bl = block_stack.back(); cur_bl; cur_bl = cur_bl->previous) {
                if (cur_bl->this_rank)
                    rinsert(*cur_bl->this_rank, plptr->rank);
                ins_pl(*plptr);
                plptr = &(cur_bl->plist);
            }
        }
    public:
        void clean_property() {
            for (auto it = name_type_map.begin(); it != name_type_map.end(); )
                if (it->second.index() >= 3)
                    it = name_type_map.erase(it);
                else ++it;
        }
        // to insert in [front_id, back_id]
        void insert_pid(property_list &pl, size_t front_id, size_t back_id)
        {
            if (front_id > back_id)
                throw except::InvalidIdRangeParseException("Such range is not acceptable");
            inner_insert_pid(pl, [front_id, back_id](combination_set &s) {
                for (size_t p = front_id; p <= back_id; ++p)
                    s.data.insert(p);
            }, [front_id, back_id](rank_vec &rv, ll rank) {
                for (size_t p = front_id; p <= back_id; ++p)
                    rv.data.emplace_back(p, rank);
            });
        }
        
        void insert_pid(property_list &pl, size_t pid) {
            inner_insert_pid(pl, [pid](combination_set &s) {
                s.data.insert(pid);
            }, [pid](rank_vec &rv, ll rank) {
                rv.data.emplace_back(pid, rank);
            });
        }
        
        /**
         * if the appended has appeared before but its id is ^larger^
         * than the number set(ignore_range), it will does nothing
         * in other cases, if the production has been appended
         * an exception would be thrown
         *
         * or, if specified, it can ignore it
         * must have a semantic process
         *
         * if appending failed, return 0
         */
        size_t append_production(const Production &p,
                                 shared_ptr<string> sp,
                                 const size_t ignore_range = -1)
        {
            // reject harmful production
            if (p.right.size() == 1 && p.right.front() == p.left) return 0;
            ps.emplace_back(p, sp);
            ps.back().first.pid = ps.size() - 1;
            e_production ep(ps.back().first);
            const auto it = production_distinct_set.insert(ep);
            if (it.second)
                return ps.size() - 1;
            
            if (cmd.ignore_duplicate_production || it.first->p.pid > ignore_range) {
                ps.pop_back();
                return 0;
            }
            
            generateException
                <except::DoubleProductionParseException>
            ("The same production is appeared", fi.getOrigin());
            return 0;
        }
        
        shared_ptr<combination_set> create_combination_set(bool left) {
            return shared_ptr<combination_set>(new combination_set(left, crtable));
        }
        
        shared_ptr<rank_vec> create_rank_vec() {
            return shared_ptr<rank_vec>(new rank_vec(crtable));
        }
        
        shared_ptr<block_info> create_block(optional<string> name, shared_ptr<combination_set> cset, shared_ptr<rank_vec> this_rank) {
            const size_t BSIZE = block_stack.size();
            while (block_name_mark.size() <= BSIZE)
                block_name_mark.push_back(0);
            shared_ptr<block_info> r(new block_info(block_stack.empty() ? nullptr : block_stack.back(), this_rank, BSIZE, block_name_mark[BSIZE]++, ps));
            if (cset)
                r->plist.csets.insert(cset);
            if (name) {
                check_name(*name);
                name_type_map[*name] = named_types::block{r->front_id, &r->back_id};
            }
            block_stack.push_back(r);
            return r;
        }
        
        pass_info(FileInteractor &fi, CommandObject &cmd, Logger &logger) noexcept: fi(fi), cmd(cmd), logger(logger), crtable(*this) {
            ps.emplace_back(Production(0), DEFAULT_OBJECT_TYPE_PROCESS_NAME);
        }
        
        // call to check if this name has already been used
        void check_name(const string &name) {
            if (name_type_map.find(name) != name_type_map.end())
                generateException<except::DoubleNameParseException>(string("Double name: ") += name, fi.getOrigin());
        }
        
        symbol get_nonterminate(const string &name) {
            auto it = name_type_map.find(name);
            if (it != name_type_map.end()) {
                if (it->second.index() == 1)
                    return std::get<named_types::nsymbol>(it->second).sid;
                else
                    generateException<except::DoubleNameParseException>(string(name) += "is not a name of nonterminate", fi.getOrigin());
            }
            else {
                name_type_map[name] = named_types::nsymbol{next_nonterminate};
                return next_nonterminate--;
            }
            // will always not reach here, this is just for compilation
            // because the compiler can't check generateException will always
            // throw an exception
            return 0;
        }
        
        symbol insert_nonterminate(const string &name) {
            check_name(name);
            name_type_map[name] = named_types::nsymbol{next_nonterminate};
            return next_nonterminate--;
        }
        
        symbol insert_anonym_nonterminate() {
            annonym_syms.insert(next_nonterminate);
            return next_nonterminate--;
        }
        
        void reverse_anonym_nonterminate() {
            ++next_nonterminate;
            annonym_syms.erase(next_nonterminate);
        }
        
        // remember to release this memory
        // pair<to_use, root>
        // to_use and root point to the same memory
        // the root is to release
        pair<const string * const, const string * const> generate_sym_list() const {
            const size_t offset = -next_nonterminate; // amount of nonterminate + start_symbol
            // (amount of nonterminate + start) + amount of terminate + eof
            string *sym_name = new string[offset + ter_sym_name_map.size() + 1];
            for (const auto &p: ter_sym_name_map)
                sym_name[p.second + offset] = p.first;
            for (const auto &p: name_type_map)
                if (p.second.index() == 1)
                    sym_name[std::get<named_types::nsymbol>(p.second).sid + offset] = p.first;
            for (const auto &s: annonym_syms)
                sym_name[offset + s] = (string("$__ann_") += std::to_string(-s));
            
            sym_name[0] = "$start$";
            sym_name[offset + ter_sym_name_map.size()] = "$eof$";
            
            return make_pair(sym_name + offset, sym_name);
        }
        
        // pair<to_use, root>
        pair<const string * const, const string *const> generate_nonterminate_cpp_name_map() const {
            const size_t offset = -cpp_type_map_min_symbol;
            string *r = new string[offset];
            auto cppt = nontermiante_cpp_type_map + cpp_type_map_min_symbol;
            for (size_t i = 0; i < offset; ++i)
                r[i] = cppt[i].first;
            return make_pair(r + offset, r);
        }
        
        /* The following can only be called once for outside */
        
        symbol get_start() {
            return next_nonterminate;
        }
        
        long long get_min_with_cpp() {
            return cpp_type_map_min_symbol;
        }
        
        vector<const Production> get_productions_in_vec() {
            vector<const Production> r;
            
            ps.front().first.left = next_nonterminate;
            auto pit = ps.begin();
            ++pit;
            ps.front().first.right.push_back(pit->first.left);
            ps.front().first.right.push_back(ter_sym_name_map.size());
            
            for (auto it = ps.begin(); it != ps.end(); ++it)
                r.emplace_back(std::move(it->first));
            
            return r;
        }
    }; // end struct
    
}


#endif /* InfoParser_hpp */
