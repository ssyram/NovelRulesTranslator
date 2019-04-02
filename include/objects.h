//
//  objects.h
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/19.
//  Copyright © 2019 ssyram. All rights reserved.
//

#ifndef objects_h
#define objects_h

#include <unordered_map>
#include <optional>
#include <string>

#include <vector>
#include "GlobalInfo.h"
#include "../except/InnerTranslateException.h"

namespace rules_translator {
    
    /**
     * this struct means the one that to specify user command
     */
    struct CommandObject {
        // not to set, if this is true, all threads should stop right away when checked this
        bool stop = false;
        /* Parse Options */
        bool no_parse_range_warn = false;
        // whether to allow nonterminate inside a production to be without '\'' or '\"'
        bool allow_nonformat_terminate = true;
        // to ignore duplication production
        bool ignore_duplicate_production = false;
        // whether the specify conflict resolving includes auto-generated productions
        bool include_auto_generate_for_conflict_resolving = false;
        // whether the property list also for auto-generated productions
        bool property_list_for_auto_generate = false;
        // whether to let anonym (auto-generated) productions do batch ("front" or "back")
        bool do_batch_for_anonym = true;
        
        bool show_all_collisions = false;
    };
    
    using symbol = long long;
    using std::vector;
    
    struct Production {
        symbol left;
        std::vector<symbol> right;
        size_t pid;
        
        Production(symbol left, size_t pid = 0): left(left), pid(pid) { }
        Production(Production &&p): left(p.left), right(std::move(p.right)), pid(p.pid) { }
        Production(const Production &) = default;
        bool operator==(const Production &p) const {
            return pid == p.pid;
        }
    };
    
    class ProductionWithDoc {
        ProductionWithDoc(const vector<symbol>::const_iterator &doc, const Production &p): doc(doc), p(p) { }
    public:
        const vector<symbol>::const_iterator doc;
        ProductionWithDoc(const ProductionWithDoc&) = default;
        void operator=(const ProductionWithDoc&) = delete;
        const Production &p;
        ProductionWithDoc(const Production &p): doc(p.right.begin()), p(p) { }
        ProductionWithDoc(ProductionWithDoc &&pwd): doc(pwd.doc), p(pwd.p) { }
        vector<symbol>::const_iterator end_doc() const {
            return p.right.end();
        }
        bool end() const {
            return doc == p.right.end();
        }
        bool last() const {
            if (end()) throw except::AccessingInvalidSymbolInPWDInnerException();
            return doc + 1 == p.right.end();
        }
        ProductionWithDoc next() const {
            if (end()) throw except::AccessingInvalidSymbolInPWDInnerException();
            return ProductionWithDoc{doc + 1, p};
        }
        symbol getNextSymbol() const {
            if (end())
                throw except::AccessingInvalidSymbolInPWDInnerException();
            return *doc;
        }
        vector<symbol>::const_iterator getFollowingSymbols() const {
            if (last()) throw except::AccessingInvalidSymbolInPWDInnerException();
            return doc + 1;
        }
        bool operator==(const ProductionWithDoc &pwd) const {
            return doc == pwd.doc && p.pid == pwd.p.pid; // end() may be the same
        }
    };
    
//    class ProductionWithDoc {
//        const size_t doc_pos;
//        const Production &p;
//    public:
//        ProductionWithDoc(const Production &p): p(p), doc_pos(0) { }
//        ProductionWithDoc(const Production &p, size_t doc_pos): p(p), doc_pos(doc_pos) { }
//        bool end() const {
//            return doc_pos == p.right.size();
//        }
//        bool last() const {
//            return doc_pos + 1 == p.right.size();
//        }
//        ProductionWithDoc next() const {
//            if (end()) throw except::InnerTranslateException("no next ProductionWithDoc.");
//
//            return ProductionWithDoc(p, doc_pos + 1);
//        }
//        symbol getNextSymbol() const {
//            if (end()) throw except::InnerTranslateException("do not have next symbol.");
//            return p.right[doc_pos];
//        }
//        std::vector<symbol>::const_iterator getFollowingSymbol() const {
//            if (last() || end()) throw except::InnerTranslateException("do not have following symbols.");
//            std::vector<symbol> r;
//            for (size_t i = doc_pos; i < p.right.size(); ++i)
//                r.push_back(r[i]);
//            return r;
//        }
//        bool operator==(const ProductionWithDoc &pwd) const {
//            return pwd.p == p && pwd.doc_pos == doc_pos;
//        }
//    };
    
    using ll = long long;
    
    class CounselTable {
        const size_t line_size;
        std::vector<ll *> table;
    public:
        CounselTable(size_t lineSize): line_size(lineSize) { }
        ~CounselTable() {
            for (auto &l: table)
                if (l) delete [] l;
        }
        ll *operator[](size_t line) {
            while (line >= table.size())
                table.push_back(new ll[line_size]());
            return table[line];
        }
        size_t lineAmount() const {
            return table.size();
        }
        size_t columnAmount() const {
            return line_size;
        }
    };
    
    class GotoTable {
        CounselTable table;
    public:
        GotoTable(size_t lineSize): table(lineSize) { }
        class LineProxy {
            friend class GotoTable;
            ll *line;
            ll min_sym;
            LineProxy(ll *line, ll line_size): line(line), min_sym(-line_size + 1) { }
        public:
            ll &operator[](symbol s) {
                if (s >= 0 || s < min_sym)
                    throw except::OutOfRangeInnerException("The symbol type is out of range of GotoTable");
                return line[-s - 1];
            }
        };
        LineProxy operator[](size_t line_num) {
            return LineProxy(table[line_num], table.columnAmount());
        }
        CounselTable &getTable() {
            return table;
        }
    };
    
    class ActionTable {
        CounselTable table;
    public:
        ActionTable(size_t lineSize): table(lineSize) { }
        class LineProxy {
            friend class ActionTable;
            ll *line;
            size_t line_size;
            LineProxy(ll *line, size_t line_size): line(line), line_size(line_size) { }
        public:
            ll &operator[](symbol s) {
                // after test, these two lines can be removed
                if (s >= line_size || s < 0)
                    throw except::OutOfRangeInnerException("The symbol type is out of range");
                return line[s];
            }
        };
        LineProxy operator[](size_t line_num) {
            return LineProxy(table[line_num], table.columnAmount());
        }
        CounselTable &getTable() {
            return table;
        }
    };
    
    using std::unordered_map;
    using std::optional;
    
    using std::nullopt;
    
    enum class conflict_type {
        reduce_shift = 1,
        shift_reduce = 2,
        reduce_reduce = 4
    };
    
    extern const char * const conflict_type_name_list[5];
    
    class ConflictResolveCounselTable {
    public:
        const std::unordered_map<size_t, std::unordered_map<size_t, uint8_t>> data;
        
        ConflictResolveCounselTable(std::unordered_map<size_t, unordered_map<size_t, uint8_t>> &&data):
            data(std::move(data)) { }
        ConflictResolveCounselTable(ConflictResolveCounselTable &&table):
            data(std::move(table.data)) { }
        
        /**
         * to quire if there is an resolving solution to
         * the specified two productions
         *
         * returns: if no info, returns nullopt
         *          true for pid1 wins, false for pid2 wins
         */
        optional<bool> resolve(size_t pid1, size_t pid2, conflict_type type) const {
            bool swap = pid1 > pid2;
            if (swap) {
                if (uint8_t(type) < 4) type = conflict_type(uint8_t(type) ^ 3);
                std::swap(pid1, pid2);
            }
            
            auto f1 = data.find(pid1);
            if (f1 == data.end()) return nullopt;
            auto f2 = f1->second.find(pid2);
            if (f2 == f1->second.end()) return nullopt;
            if (!(f2->second & ((uint8_t)type << 4))) return nullopt;
            return swap ? (f2->second & (uint8_t)type) : !(f2->second & (uint8_t)type);
        }
    };
    
    using std::string;
    
    struct ParseInfo {
        const symbol eof, start; // eof means the largest one, start means the smallest one
        const long long min_with_cpp; // the smallest one with cpptype
        const string * const type_map; // type_map[symbol];
        const string * const nttype_cppname_map; // nttype_cppname_map[symbol];
        const ConflictResolveCounselTable crtable;
        const vector<Production> ps;
        ~ParseInfo() {
            const auto s = nttype_cppname_map + min_with_cpp; // "min_with_cpp" is minus
            delete []s;
            const auto t = type_map + start; // "start" is minus
            delete []t;
        }
    };
}

#endif /* objects_h */
