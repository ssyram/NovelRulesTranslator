//
//  InfoParser.cpp
//  NovelRulesTranslator
//
//  Created by 潇湘夜雨 on 2019/2/25.
//  Copyright © 2019 ssyram. All rights reserved.
//

#include "InfoParser.hpp"

const static bool not_require = false;
const static bool check_match = true;

#define nf "Not finish."
#define wo "Working On."

using ll = long long;

namespace rules_translator {
    
    using namespace utils;
    using namespace fi_fn;
    using namespace generator;
    using namespace container;
    
    const static char *FRONT_TYPE_PARA_LIST = "(const ll left_type, std::vector<symbol_type> &right, pass_info &info)";
    const static char *BACK_TYPE_PARA_LIST = "(symbol_type &left, std::vector<symbol_type> &right, pass_info &info)";
    const static char *FRONT_CALL = "(left_type, right, info);";
    const static char *BACK_CALL = "(left, right, info);";
    const static char *PASS_INFO = "pass_info";
    const static char *VOID_PREFIX = "void ";
    const static char *DEFAULT_OBJECT_TYPE = "default_object_type";
    const static char *TOKEN_TYPE = "token_type";
    const static char *OBJECT_TYPE = "object_type";
    const static char *SYMBOL_TYPE = "symbol_type";
    const static char *ITER_OBJECT_TYPE = "iter_object_type";
    const char *INIT_AGACP = "__initial_auto_generated_acproduction_";
    const char *LEFT_COMBINE_AGACP = "__left_combine_auto_generated_acproduction_";
    const char *RIGHT_COMBINE_AGACP = "__right_combine_auto_generated_acproduction_";
    const char *DEFAULT_LEFT_PROCESS_PREFIX = "__default_left_process_";
    
    const shared_ptr<string> raw_ac_production::INIT_AGAC(new string(INIT_AGACP));
    const shared_ptr<string> raw_ac_production::LEFT_AGAC(new string(LEFT_COMBINE_AGACP));
    const shared_ptr<string> raw_ac_production::RIGHT_AGAC(new string(RIGHT_COMBINE_AGACP));
    shared_ptr<string> DEFAULT_OBJECT_TYPE_PROCESS_NAME(new string("__default_object_type_semantic_process_"));
    
	enum class fix_genre {
		prefix,
		postfix,
		nothing
	};

    void ConflictResolveSettingTable::generate_exception(size_t pid1, size_t pid2, rules_translator::conflict_type &type) {
        
        auto p = info.generate_sym_list();
        const string * const sym_list = p.first;
        
        // get the two
        Production *p1 = nullptr, *p2 = nullptr;
        {
            size_t count = 0, max = pid2 > pid1 ? pid1 : pid2;
            for (auto &p: info.ps) {
                if (count == pid1) p1 = &p.first;
                if (count == pid2) p2 = &p.first;
                if (count == max) goto brk;
                ++count;
            }
            throw except::NoSuchProductionInnerException(string("No such production as id = ") += (pid1 > pid2 ? pid1 : pid2));
        }
    brk:
        
        auto print_symbol = [sym_list](const symbol s, ostream &os) -> ostream &{
            if (s >= 0)
                return os << (quoted(sym_list[s], '\''));
            else return os << sym_list[s];
        };
        
        auto print_production = [&print_symbol](const Production &p1, ostream &buffer) {
            buffer << "Production: ";
            print_symbol(p1.left, buffer) << " := ";
            for (const auto &s: p1.right)
                print_symbol(s, buffer) << " ";
            buffer << endl;
        };
        
        stringstream buffer;
        buffer << "Cannot set conflict resolving table, for:" << endl;
        print_production(*p1, buffer);
        buffer << "and" << endl;
        print_production(*p2, buffer);
        buffer << "for: conflict " << quoted(conflict_type_name_list[uint8_t(type)]) << endl;
        
        delete [] p.second;
        generateException<except::SpecifyingConflictParseException>(buffer.str(), info.fi.getOrigin());
    } // end member function ConflictResolveSettingTable::generate_exception
    
    std::shared_ptr<ParseInfo>
    parse(
          FileInteractor &fi,
          CommandObject &cmd,
          utils::Logger &logger)
    {
        pass_info info(fi, cmd, logger);
        
        string buffer;
        fi_info infobuffer;
        // get terminate
        {
            fi.expect(expect("terminate"));
            fi.expect(expect("="));
            fi.expect(expect("{"));
            
            symbol next_terminate = 0;
            
            // insert the terminate with specified name
            auto insert_terminate = [&buffer, &infobuffer, &next_terminate, &info](const string &name) {
                if (info.ter_sym_name_map.find(name) != info.ter_sym_name_map.end())
                    generateException<except::DoubleNameParseException>(string("redefine of a name of terminte : ") += name, info.fi.getOrigin());
                info.ter_sym_name_map[name] = next_terminate++;
            };
            
            // get each of the terminate, must at least has one
            // if read nothing, throw exception automatically
            // read the first one
            // read the terminate: if get an identifier, return that, if not, get a literal
            buffer = fi.read(generator::get_literal(expect<check_match>(object::get_identifier, fi_type::identifier)));
            insert_terminate(buffer);
            while (1) {
                if (!fi.expect(expect(","), not_require)) break;
                infobuffer = fi.read_info(generator::get_literal(expect<check_match>(object::get_identifier, fi_type::identifier)), not_require);
                if (infobuffer.type == fi_type::nothing) break;
                insert_terminate(infobuffer.content);
            }
            fi.expect(expect("}"));
            fi.expect(expect(";"), not_require);
#ifdef TEST
            logger.logln("After get terminate:");
            logger.logln(info.ter_sym_name_map.size(), " tokens:");
            for (auto &p: info.ter_sym_name_map)
                logger.log(p.first, ": ", std::to_string(p.second));
            logger.logln("---------------------------------------");
#endif
        }
        
        // get fixed part
        {
            // token_type
            {
                fi.expect(expect("token_type"));
                fi.expect(expect("="));
                buffer = fi.read(object::get_formatted_cpp_code_to_semicolon);
                fi.write("using token_type = ", buffer, ";");
#ifdef TEST
                logger.logln("token_type mode end: token_type: ", buffer);
#endif
            }
            // get_type
            {
                fi.expect(expect("get_type"));
                fi.expect(expect("="));
                if (fi.expect(expect("{"), not_require)) { // define function directly
                    buffer = fi.read(object::get_cpp_block);
                    
                    // translate the content in this block
                    string to_write;
                    const auto SIZE = buffer.size();
                    for (size_t i = 0; i < SIZE; ++i) {
                        if (buffer[i] == '$' && (buffer[i + 1] == '\'' ||
                                                 buffer[i + 1] == '"')) {
                            const char TAR = buffer[i + 1];
                            i += 2;
                            bool ignore = false;
                            auto begin = i;
                            for (; i < SIZE && (ignore || buffer[i] != TAR); ++i)
                                if (buffer[i] == '\\' && !ignore) ignore = true;
                                else ignore = false;
                            if (i >= SIZE)
                                generateException
                                    <except::UnterminatedLiteralParseException>
                                        ("Unterminated Literal", fi.getOrigin());
                            
                            string temp = buffer.substr(begin, i - begin);
                            const auto it = info.ter_sym_name_map.find(temp);
                            if (it == info.ter_sym_name_map.end())
                                generateException
                                    <except::TokenNotRegisteredParseException>
                                        ("This is not a Registered Terminate Symbol", fi.getOrigin());
                            to_write += std::to_string(it->second);
                        }
                        else to_write += buffer[i];
                    }
                    
                    fi.writeln("size_t get_type(const token_type &token) {")
                    .writeln(to_write)
                    .writeln("}");
                    fi.expect(expect(";"), not_require);
#ifdef TEST
                    logger.logln("get_type mode: block, content:");
                    logger.logln(buffer);
#endif
                }
                else {
                    buffer = fi.read(object::get_formatted_cpp_code_to_semicolon);
                    fi.writeln("size_t (*get_type)(const token_type &) = ", buffer, ";");
#ifdef TEST
                    logger.logln("get_type mode: name, content:");
                    logger.logln(buffer);
#endif
                }
            }
            // pass_info_class
            {
                if (fi.expect(expect("pass_info_struct"), not_require)) {
                    fi.expect(expect("="));
                    if (fi.expect(expect("{"), not_require)) { // to determine a class here
                        buffer = fi.read(object::get_cpp_block);
                        fi.writeln("struct pass_info {")
                        .writeln(buffer)
                        .writeln("};");
                        fi.expect(expect(";"), not_require);
                    }
                    else { // just bind a name
                        buffer = fi.read(object::get_formatted_cpp_code_to_semicolon);
                        fi.writeln("using pass_info = ", buffer, ";");
                    }
                }
                else {
                    fi.writeln("struct pass_info { };");
                }
            }
        }
        
        // using part
        {
            unordered_set<string> cpptypes;
            vector<string> vs; // as the temp one for cpp_type_map
            while (fi.expect(expect("using"), not_require)) {
                buffer = fi.read(expect<check_match>(object::get_identifier, fi_type::identifier));
                
                // insert a name
                info.insert_nonterminate(buffer);
                
                fi.expect(expect("="));
                infobuffer.content = fi.read(object::get_formatted_cpp_code_to_semicolon);
                cpptypes.insert(infobuffer.content);
                vs.emplace_back(std::move(infobuffer.content));
            }
            // create cpp_type_map
            {
                const size_t SIZE = vs.size();
                info.cpp_type_map_min_symbol = -intptr_t(SIZE);
                info.nontermiante_cpp_type_map = new pair<string, shared_ptr<string>>[SIZE];
                for (size_t i = vs.size() - 1; i < SIZE; --i)
                    info.nontermiante_cpp_type_map[SIZE - i - 1] = make_pair(std::move(vs[i]), nullptr);
                info.nontermiante_cpp_type_map += SIZE;
            }
            
            fi.write("struct ", DEFAULT_OBJECT_TYPE, "{ };");
            fi.write("struct ", ITER_OBJECT_TYPE, ";");
            fi.write("using ", OBJECT_TYPE, " = std::variant<");
            for (auto &s: cpptypes)
                fi.write(s).write(", ");
            fi.writeln(DEFAULT_OBJECT_TYPE, ", ", TOKEN_TYPE, ", ", ITER_OBJECT_TYPE, ">;");
#ifdef TEST
            logger.logln("Finish using part")
            .logln("The got names:");
            for (auto &p: info.name_type_map)
                logger.log(p.first).log(": ")
                .logln(std::to_string(std::get<named_types::nsymbol>(p.second).sid));
#endif
        }
        
        // generate start data
        {
            fi.writeln("namespace utils { extern const std::string * const type_name_map; }");
            fi.writeln("using ll = long long;");
            fi.write("struct ", SYMBOL_TYPE, ";");
            fi.writeln("struct ", ITER_OBJECT_TYPE, " {", "std::vector<", SYMBOL_TYPE, ">", " data;", "};");
            fi.writeln("struct ", SYMBOL_TYPE, "{")
            .writeln(" ll type; object_type object; ")
            .writeln(SYMBOL_TYPE, "() = default;")
            .writeln(SYMBOL_TYPE,
                     "(ll type, object_type &&object): type(type), object(std::move(object)) { }")
            .writeln(SYMBOL_TYPE, "&operator=(symbol_type &&s) {")
            .writeln("type = s.type; object = std::move(s.object); return *this; };")
            .writeln("void operator=(const symbol_type &) = delete;")
            .writeln("void operator=(symbol_type &) = delete;")
            .writeln("symbol_type(const symbol_type &) = default;")
            .writeln("};");
            fi.writeln("template <typename ...Ts> struct overloaded: Ts... { using Ts::operator() ...; };")
            .writeln("template <typename ...Ts> overloaded(Ts...) -> overloaded<Ts...>;");
            auto output_semantic_process =
            [&fi](const string_view &name, std::function<void (FileInteractor &)> core_call) {
                core_call(fi.write(SYMBOL_TYPE)
                          .write(" ").write(name)
                          .write(FRONT_TYPE_PARA_LIST)
                          .writeln(" {"));
                fi.writeln("}");
            };
            output_semantic_process(*DEFAULT_OBJECT_TYPE_PROCESS_NAME, [](FileInteractor &fi) {
                fi.writeln("return ", SYMBOL_TYPE, "{ left_type, ", DEFAULT_OBJECT_TYPE, "{} };");
            });
            output_semantic_process(INIT_AGACP, [](FileInteractor &fi) {
                fi.writeln(ITER_OBJECT_TYPE, " left;")
                .writeln("for (auto it = right.begin(); it != right.end(); ++it)")
                .writeln("left.data.emplace_back(*it);")
                .writeln("return ", SYMBOL_TYPE, "{ left_type, std::move(left) };");
            });
            output_semantic_process(LEFT_COMBINE_AGACP, [](FileInteractor &fi) {
                fi.writeln
                (ITER_OBJECT_TYPE,
                 " left{std::move(std::get<iter_object_type>(right.front().object).data)};")
                .writeln("for (auto it = right.begin() + 1; it != right.end(); ++it)")
                .writeln("left.data.emplace_back(*it);")
                .writeln("return ", SYMBOL_TYPE, "{ left_type, std::move(left) };");
            });
            output_semantic_process(RIGHT_COMBINE_AGACP, [](FileInteractor &fi) {
                fi.writeln
                (ITER_OBJECT_TYPE,
                 " left{std::move(std::get<iter_object_type>(right.back().object).data)};")
                .writeln("const auto END = right.end() - 1;")
                .writeln("for (auto it = right.begin(); it != END; ++it)")
                .writeln("left.data.emplace_back(*it);")
                .write("return ", SYMBOL_TYPE, "{ left_type, std::move(left) };");
            });
        }
        
        
        
        /* block analyze */
        
        
        
        // block part, because it will call itself, so it's a func
        // if there is a name, it should be put into
        // get the pointer to current block from info.block_stack.back()
        // outside must put and specify the combination and rank list(this_rank).
        std::function<void ()> block_analyze = [&buffer, &infobuffer, &info, &fi, &block_analyze, &logger] () {
#ifdef TEST
            logger.logln("Starts a new block");
#endif
            // tool functions
            /**
             * the specified property list will
             * tell almost all the context-free content
             * except for the previous rank info
             *
             * params:
             *      pl: the property list to fill
             *      pre_this_rank: to tell if needs to get rank
             */
            auto property_list_fill =
                [&buffer, &infobuffer, &info, &fi](property_list &pl, shared_ptr<rank_vec> pre_this_rank)
            {
                bool set = false;
            cont:
                if (pre_this_rank && !set) {
                    infobuffer = fi.read_info(object::get_integer, not_require);
                    if (infobuffer.type != fi_type::nothing) {
                            pl.rank = utils::string2ll(infobuffer.content);
                        set = true;
                    }
                }
            set_cont:
                infobuffer = fi.read_info(expect<check_match>(object::get_identifier, { fi_type::identifier, fi_type::keyword_left, fi_type::keyword_right, fi_type::keyword_nofront,
                    fi_type::keyword_noback
                }), not_require);
                if (infobuffer.type == fi_type::nothing) goto ret;
                
                if (infobuffer.type == fi_type::identifier) {
                    visit([&fi, &info, &pl, &infobuffer](auto &&t) {
                        using T = std::decay_t<decltype(t)>;
#define con(type) if constexpr (std::is_same_v<T, std::decay_t<named_types::type>>)
                        con(combination) {
                            pl.csets.insert(t.ptr);
                        }
                        else con(rank) {
							infobuffer = fi.read_info(object::get_integer, not_require);
							if (infobuffer.type != fi_type::nothing)
								pl.rvec_info.emplace(t.ptr, utils::string2ll(infobuffer.content));
                            else pl.rvec_info.emplace(t.ptr, t.ptr->get_next_rank());
                        }
                        else con(mix_property) {
                            pl.csets.insert(t.cptr);
							infobuffer = fi.read_info(object::get_integer, not_require);
							if (infobuffer.type != fi_type::nothing)
								pl.rvec_info.emplace(t.rptr, utils::string2ll(infobuffer.content));
							else pl.rvec_info.emplace(t.rptr, t.rptr->get_next_rank());
                        }
                        else
                            generateException
                                <except::UnexpectedTokenParseException>
                                    ("This name is not a name of combination, rank or mix property", fi.getOrigin());
#undef con
                    }, info.name_type_map[infobuffer.content]);
                }
                else if (infobuffer.type == fi_type::keyword_nofront)
                    pl.nofront = true;
                else if (infobuffer.type == fi_type::keyword_noback)
                    pl.noback = true;
                else {
                    if (pl.sctype != property_list::strong_combination_type::not_set)
                        generateException
                            <except::DoubleSetStrongCombinationParseException>
                                ("The strong combination is set for more than one time in a property list", fi.getOrigin());
                    if (infobuffer.type == fi_type::keyword_left)
                        pl.sctype = property_list::strong_combination_type::left;
                    else pl.sctype = property_list::strong_combination_type::right;
                }
                
                if (set) goto set_cont;
                goto cont;
            ret:
                if (!pre_this_rank || set) return;
                pl.rank = pre_this_rank->get_next_rank();
            };
            /**
             * params:
             *      name: the name of the semantic function
             *      of_back_type: should it pass both left and right or just pass right
             *      pre_func: the prefix or postfix function
             */
            auto semantic_fill = // after pro_equ
                [&buffer, &info, &fi](const string &name, bool of_back_type, optional<string> pre_func)
            {
                const char *para_list = of_back_type ? BACK_TYPE_PARA_LIST : FRONT_TYPE_PARA_LIST;
                const char *call = of_back_type ? BACK_CALL : FRONT_CALL;
                bool block = fi.expect(expect("{"), not_require);
                if (!block) { // declare this function
                    buffer = fi.read(object::get_formatted_cpp_code_to_semicolon);
                    fi.write((string(VOID_PREFIX) += buffer) += para_list).writeln(";");
                }
                else {
                    buffer = fi.read(object::get_cpp_block);
                    fi.expect(expect(";"), not_require);
                }
                
                fi.write(VOID_PREFIX, name, para_list, " {");
                if (pre_func && !of_back_type)
                    fi.writeln(*pre_func, FRONT_CALL);
                if (!block) fi.writeln(buffer += call);
                else fi.writeln(buffer);
                if (pre_func && of_back_type)
                    fi.writeln(*pre_func, BACK_CALL);
                fi.writeln("}");
            };
            
            // choose part analyze
            // method of rail changing
            // only reads a token a time, then dispatch it
            symbol temp_left = 0; // to check if there is a temp_left -- if (temp_left)
            const static char *SEMANTIC_PROCESS_PREFIX = "__process_";
            
// here in order to prevent the intendent to become too long
/**
 * get and push a production starts from the first right symbol
 * "set" means that to get all of the continuous productions
 * the first name also must be passed, because there is no way to know
 * if it's an auto-combine production
 * that is to know: [<first_name> ':'] <left> ':='
 *
 * functionality description:
 *      analyze all the production follows this one that ^has no name^ and starts with '|'
 */
auto get_production_set = [&info, &infobuffer, &fi, &property_list_fill](const symbol left, optional<string> first_name) {
    
    
    
    
/**
 * this may change to acproduction analyze
 * this one should also push the production and default process
 *
 * the first set will be filled with empty
 * while the following will need to see whether it needs to modify
 *
 * params:
 *      (Production &)p: the production with left defined
 * returns:
 *      end of the first set
 */
auto read_right = [&info, &infobuffer, &fi](Production &p) -> size_t {

fix_genre fix;
auto read_token = [&infobuffer, &fix, &fi](bool normal/* Mark if it isn't an auto-combine production */) -> bool {
    fix = fix_genre::nothing;
    // this function makes sure that all symbol and condition come out from it is valid.
    infobuffer = fi.read_info
    ([&fix, &infobuffer, &fi, normal](fi_type &type, string &s, istream &is) -> bool {
        // essentially, the prefix is one token, and should be analyzed in total
        // so, here, analyze the whole token
        /* Contemporarily, the functionality of duplication specification will not be implemented. */
        // optional<pair<optional<uint32_t>, optional<uint32_t>>> fix;
        /*
         // when '(' is got, call this function to get duplication time specification info
         // after this function, the pointer will point to char just after ')'
         auto get_to_fix = [&fi, &infobuffer]() -> pair<optional<uint32_t>, optional<uint32_t>> {
         infobuffer = fi.read_info(sequence({
         object::get_unsigned_integer, expect<check_match>(object::get_sym, {
         fi_type::sym_right_brace, fi_type::sym_to
         })
         }));
         fi_type &type = infobuffer.type;
         pair<optional<uint32_t>, optional<uint32_t>> to_fix;
         if (type == fi_type::integer) {
         to_fix.first = (uint32_t)utils::string2size_t(infobuffer.content);
         infobuffer = fi.read_info(expect(object::get_sym, {
         fi_type::sym_right_brace, fi_type::sym_to
         }));
         }
         if (type == fi_type::sym_to) {
         infobuffer = fi.read_info(sequence({
         object::get_unsigned_integer,
         expect<check_match>(object::get_sym, fi_type::sym_right_brace)
         }));
         if (type == fi_type::integer) {
         to_fix.second = (uint32_t)utils::string2size_t(infobuffer.content);
         fi.expect(expect(")"));
         }
         }
         return to_fix;
         };
         if (is.peek() == '(') {
         is.get();
         auto to_fix = get_to_fix();
         fi.expect(expect("*"));
         fix = fix_genre::prefix;
         }
         else
         */
        if (is.peek() == '*') {
            is.get();
            fix = fix_genre::prefix;
        }
        if (expect<check_match>(object::get_identifier, fi_type::identifier)(type, s, is)) goto back_check;
        if (object::get_literal(type, s, is)) goto back_check;
        if (normal) {
            if (!expect(object::get_sym, {
                fi_type::sym_left_brace, fi_type::sym_left_sqrbkt
            })(type, s, is))
                return false;
        }
        else if (!expect(object::get_sym, {
            fi_type::sym_left_brace, fi_type::sym_left_sqrbkt,
            fi_type::sym_or, fi_type::sym_right_brace, fi_type::sym_right_sqrbkt
        })(type, s, is))
            return false;
    back_check:
        if (fix == fix_genre::prefix) {
            if (!normal &&
                (type == fi_type::sym_or ||
                 type == fi_type::sym_right_brace ||
                 type == fi_type::sym_right_sqrbkt)
                )
                generateException
                    <except::UnexpectedTokenParseException>
                        ("Here does not allow prefix '*'", is);
        }
        else if (is.peek() == '*' && fix == fix_genre::nothing) {
            is.get();
            fix = fix_genre::postfix;
            if (type == fi_type::sym_or ||
                type == fi_type::sym_left_brace ||
                type == fi_type::sym_left_sqrbkt
                )
                generateException
                    <except::UnexpectedTokenParseException>("Here does not allow '*'", is);
            /* The following are codes for contemporarily deprecated mission: duplication time specification */
            /*
             if (is.get() == '(')
             auto to_fix = get_to_fix();
             else is.unget();
             */
        }
        return true;
    }, not_require);
    
    
    if (infobuffer.type == fi_type::nothing || infobuffer.type == fi_type::finished)
        return false;
    return true;
}; // end read_token
auto get_symbol_from_identifier = [&info](const string &buffer) ->symbol {
    const auto it = info.name_type_map.find(buffer);
    if (it == info.name_type_map.end()) {
        if (info.cmd.allow_nonformat_terminate) {
            const auto tit = info.ter_sym_name_map.find(buffer);
            if (tit == info.ter_sym_name_map.end())
                return info.insert_nonterminate(buffer);
            else
                return tit->second;
        }
        else return info.insert_nonterminate(buffer);
    }
    else if (it->second.index() != 1)
        generateException
            <except::UnexpectedTokenParseException>
                ("This is not a name of a symbol", info.fi.getOrigin());
    else return std::get<named_types::nsymbol>(it->second).sid;
    // this will not happen
    return 0;
}; // end get_symbol_from_identifier


string &buffer = infobuffer.content;
fi_type &type = infobuffer.type;
// normal rail
while (1) {
    if (!read_token(true)) {
        info.append_production(p, nullptr);
        return info.ps.size() - 1;
    }
    // check to change rail
    if (fix != fix_genre::nothing || type == fi_type::sym_left_sqrbkt || type == fi_type::sym_left_brace)
        break;
    if (type == fi_type::identifier) {
        p.right.push_back(get_symbol_from_identifier(buffer));
    }
    else { // this must be fi_type::terminate
        const auto it = info.ter_sym_name_map.find(buffer);
        if (it == info.ter_sym_name_map.end())
            generateException
                <except::TokenNotRegisteredParseException>
                    ("No such terminate symbol.", fi.getOrigin());
        p.right.push_back(it->second);
    }
}
// acproduction rail

// raw_sym is not pointer
// acp is pointer

// to determine the push range
// inside this range, shadow same check of insertion
size_t PUSH_RANGE = info.ps.size() - 1;
// pair<pointer to a raw acproduction, iter_mark stack of this acp>
stack<pair<shared_ptr<raw_ac_production>, stack<iter_mark>>> acp_stack;
auto push_symbol = [&acp_stack](raw_sym &&s) {
    auto &p = acp_stack.top();
    if (p.second.empty()) p.first->right.push_back(std::move(s));
    else p.second.top().push_sym(std::move(s));
};
auto acproduction_append = [PUSH_RANGE, &info](const Production &p, shared_ptr<string> sp) -> size_t {
    return info.append_production(p, sp, PUSH_RANGE);
};
acp_stack.push(make_pair
               (shared_ptr<raw_ac_production>
                (new raw_ac_production(p, acproduction_append)), stack<iter_mark>()));
auto single_symbol_tackle =
[&info, &acproduction_append, &acp_stack, &push_symbol]
(symbol s, fix_genre fix) {
    if (fix == fix_genre::nothing) {
        push_symbol(s);
        return;
    }
    auto r = shared_ptr<raw_ac_production>(new raw_ac_production
                                           (info.insert_anonym_nonterminate(),
                                            acproduction_append));
    iter_mark im{true};
    im.push_sym(r->left); // iterate the left
    if (fix == fix_genre::prefix) {
        r->right.push_back(s);
        r->right.push_back(std::move(im));
    }
    else {
        r->right.push_back(std::move(im));
        r->right.push_back(s);
    }
    acp_stack.top().first->temp_q.push(r);
    push_symbol(r->left);
};
// when starts, the infobuffer already has somthing to tell, for one time, tackle a single symbol
// and it should be a valid symbol
do {
    if (type == fi_type::identifier)
        single_symbol_tackle(get_symbol_from_identifier(buffer), fix);
    else if (type == fi_type::terminate) {
        const auto it = info.ter_sym_name_map.find(buffer);
        if (it == info.ter_sym_name_map.end())
            generateException
                <except::UnknownTokenParseException>
                    ("This is not a name of terminate symbol", fi.getOrigin());
        
        single_symbol_tackle(it->second, fix);
    }
    else if (type == fi_type::sym_or) {
        if (!acp_stack.top().second.empty()) {
            acp_stack.top().second.top().push_or();
        }
        else { // must be: if (acp_stack.size() == 1) -- this '|' does not belong to this part of right
            fi.getOrigin().unget();
            break;
        }
    } // end or
    else if ((size_t(type) - size_t(fi_type::sym_left_brace)) % 2) { // rights
        bool can_null = (type == fi_type::sym_right_sqrbkt);
        auto &top_stack = acp_stack.top().second;
        if (top_stack.empty() || top_stack.top().quiry_can_null() != can_null)
            generateException
                <except::BlockTypeNotMatchParseException>
                    ("This block end token does not match", info.fi.getOrigin());
        auto im = std::move(top_stack.top());
        top_stack.pop();
        if (im.inside_syms.empty()) { // avoid empty
            if (top_stack.empty() && acp_stack.size() != 1)
                acp_stack.pop();
        }
        else if (top_stack.empty() && acp_stack.size() != 1) { // to take the auto-generated acproduction back
            if (fix == fix_genre::postfix)
                generateException
                    <except::DoubleDuplicationParseException>
                        ("This block has both prefix and postfix", info.fi.getOrigin());
            auto r = acp_stack.top().first;
            // right-combination
            {
                // create a single optional symbol
                iter_mark iim{true};
                iim.push_sym(r->left);
                r->right.push_back(std::move(im));
                r->right.push_back(std::move(iim));
            }
            acp_stack.pop();
            acp_stack.top().first->temp_q.push(r);
            push_symbol(r->left);
        }
        
        
        // here is top_stack not empty or it's the root acproduction
        // if has postfix, just build up a symbol and build up a auto-generated acproduction
        // if not, just push the symbol to next position to insert into
        
        else if (fix == fix_genre::postfix) {
            auto r = shared_ptr<raw_ac_production>(new raw_ac_production
                                                   (info.insert_anonym_nonterminate(),
                                                    acproduction_append));
            // left-combination
            {
                iter_mark iim{true};
                iim.push_sym(r->left);
                r->right.push_back(std::move(iim));
                r->right.push_back(std::move(im));
            }
            acp_stack.top().first->temp_q.push(r);
            push_symbol(r->left);
        }
        else { // here can only be nothing
            push_symbol(std::move(im));
        }
    } // end rights
    else { // lefts
        bool can_null = (type == fi_type::sym_left_sqrbkt);
        if (fix == fix_genre::nothing) {
            acp_stack.top().second.emplace(can_null);
        }
        else { // here can only be prefix
            stack<iter_mark> ns;
            ns.emplace(can_null);
            acp_stack.emplace(shared_ptr<raw_ac_production>
                              (new raw_ac_production
                               (info.insert_anonym_nonterminate(),
                                acproduction_append)),
                              std::move(ns));
        }
    } // end lefts
    
    // read a symbol
    
} while (read_token(false));

// check if it's valid return
if (acp_stack.size() != 1 || !acp_stack.top().second.empty())
    generateException
        <except::UnexpectedEndParseException>
            ("Unterminated block", fi.getOrigin());

// starts to analyze all of the symbols read
size_t r = acp_stack.top().first->analyze();
return r;
}; // end read_right
    
    
#pragma message("Start Get Production Set")
    

bool brk = false;
do {
    // last iterator
    // for info.ps has at least one production, no need to worry about fit == info.ps.end()
    auto front_it = --info.ps.end();
    const size_t fid = info.ps.size(); // front_id, the first id
    Production p(left);
    // the return bid >= fid or !bid, !bid means the production is totally duplicated, which is unacceptable
    const size_t bid = read_right(p); // back_id, that's just the id of the last one of the root acproduction
    
    // if no first set production is insert, there will be no posibility for auto-generated productions
    // because every auto-generated productions will be started by a new anonym nonterminate
    // nothing is inserted means that new auto-generated productions are not possible
    if (!bid)
        generateException
            <except::DoubleProductionParseException>
                ("This production is totaly the same with the before ones", fi.getOrigin());
    
    ++front_it; // to make this iterator point to fid
    
    const size_t LAST_ID = info.ps.size() - 1;
    if (first_name) { // the name here must be a new name, because only checked name will be put into this function
        if (fid == LAST_ID)
            info.name_type_map[*first_name] = named_types::production{fid};
        else
            info.name_type_map[*first_name] = named_types::acproduction{
                fid,
                info.cmd.include_auto_generate_for_conflict_resolving ? LAST_ID : bid
            };
        first_name = nullopt; // do not pass to another set
    }
    
    // finish is condition token that can be passed, however, the next name is not
    // so must pay attention not to read the next name, only read the things that can be tackled here
    // but for finish, because it can be passed, till the end, finish should not be tackled
    
    // after right, there must be a symbol or it's already finished
    infobuffer = fi.read_info(expect<check_match>(object::get_sym, {
        fi_type::sym_comma, fi_type::sym_semicolon, fi_type::sym_equ, // belongs to this production
        fi_type::sym_or, // next production
        fi_type::sym_right_brace // end of this block
    }), not_require); // here allows finish
    property_list pl;
    // firstly, tackle property_list
    if (infobuffer.type == fi_type::sym_comma) {
        // if it's comma, here must be a property list analyze
        // even if there is no actual property specified -- it just returns nothing
        property_list_fill(pl, info.block_stack.back()->this_rank);
        // here must be symbol, has ',', ';', '|', '=', '{', '}'
        infobuffer = fi.read_info(expect<check_match>(object::get_sym, {
            fi_type::sym_comma, fi_type::sym_semicolon,     // still the same production
            fi_type::sym_left_brace, fi_type::sym_equ,      // still the same production
            fi_type::sym_or,                                // next production
            fi_type::sym_right_brace                        // end of the block
        }), not_require);
        if (infobuffer.type == fi_type::sym_comma) // from here on, it can be next production's name / nonterminate name
            infobuffer = fi.read_info(expect<check_match>(object::get_sym, {
                fi_type::sym_semicolon, fi_type::sym_or,
                fi_type::sym_left_brace, fi_type::sym_equ,
                fi_type::sym_right_brace
            }), not_require);
    }
    else
        if (info.block_stack.back()->this_rank) pl.rank = info.block_stack.back()->this_rank->get_next_rank();
    // from here on, the ',' part is finished, only symbols followed are allowed
    
    // out of here, there is the posibility that the next one is another name
    
    // here, pl is already determined
    if (fid == LAST_ID)
        info.insert_pid(pl, fid);
    else
        info.insert_pid
        (pl,
         fid,
         info.cmd.property_list_for_auto_generate ? LAST_ID : bid);
    
    // to generate all of the format of a function but not register it
    // cannot be static for it must capture something.
    /**
     * To just output the semantic process for productions
     * do not use as the process for back or front
     *
     * includes:
     *      1. header with fid as name
     *      2. front
     *      3. left define
     *      4. call core to write
     *      5. back
     *      6. return left
     *
     * params:
     *      core_call: the content to be printed
     *      fid: to determine the name of this function, name: "__process_<fid>"
     *      left_type_name: the name of the type of left
     ****** init_by_core_call: left will be initialized by core call, which means to fill content after "symbol_type left = "
     */
    auto output_production_semantic_process =
    [&info, &fi, &pl](std::function<void ()> core_call,
                      const size_t fid,
                      const string_view &left_type_name,
                      const bool init_by_core_call) {
        block_info &binfo = *info.block_stack.back();
        // to output the semantic process
        fi.writeln(SYMBOL_TYPE, " ", SEMANTIC_PROCESS_PREFIX, fid, FRONT_TYPE_PARA_LIST, " {");
        if (!pl.nofront && binfo.withfront)
            fi.writeln(binfo.get_name() += FRONT_CALL);
        
        
        // initialize left
		if (!init_by_core_call) {
            // because Clang and VC supports different kinds of not-single-symbol initialization form
            // Clang: left{left_type, (unsigned long long){} };
            // VC: left{left_type, unsigned long long{} };
            // the opposite form will cause error in the opposite side.
			fi.writeln("using $Tp = ", left_type_name, ";");
			fi.writeln(SYMBOL_TYPE, " left{ left_type, $Tp{} };");
		}
		else
            fi.write(SYMBOL_TYPE, " left = ");
        
        core_call();
        
        
        // call back()
        if (!pl.noback && binfo.withback)
            fi.writeln(binfo.get_name(), BACK_CALL);
        
        // call to finish this function
        fi.writeln("return left;");
        fi.writeln("}");
    }; // end generate_semantic_process
    
    
    
    // these two are for fid
    string name_for_left;
    if (front_it->first.left < info.cpp_type_map_min_symbol)
        name_for_left = DEFAULT_OBJECT_TYPE;
    else
        name_for_left = info.nontermiante_cpp_type_map[front_it->first.left].first;
    
    /**
     * To just output the empty semantic process --
     * if with front or back, just output it
     *
     * This function will also register this function
     * "generate" means to output and to register
     *
     * side effect: this one will move front_it
     */
    auto generate_empty_semantic_process =
    [&info, &pl, &output_production_semantic_process, fid, &name_for_left, &front_it, bid, &fi]() {
        // take out the latest block
        block_info &binfo = *info.block_stack.back();
        shared_ptr<string> r;
        if ((!pl.nofront && binfo.withfront) || (!pl.noback && binfo.withback)) {
            output_production_semantic_process([](){}, fid, name_for_left, false);
            r = shared_ptr<string>(new string
                                   (string(SEMANTIC_PROCESS_PREFIX) +=
                                    std::to_string(fid)));
        }
        else {
            if (front_it->first.left < info.cpp_type_map_min_symbol)
                r = DEFAULT_OBJECT_TYPE_PROCESS_NAME;
            else {
                auto &sp = info.nontermiante_cpp_type_map[front_it->first.left].second;
                if (!sp) {
                    sp = shared_ptr<string>(new string
                                            (string(DEFAULT_LEFT_PROCESS_PREFIX) +=
                                             std::to_string(-front_it->first.left)));
                    
                    fi.writeln(OBJECT_TYPE, *sp, FRONT_TYPE_PARA_LIST, " {")
                    .writeln("return ", name_for_left, "{};")
                    .writeln("}");
                }
                r = sp;
            }
        }
        for (size_t k = fid; k <= bid; ++front_it, ++k)
            front_it->second = r;
    }; // end generate_no_semantic_process
    
    /**
     * To fill the semantic process by fid
     * this should only be used when need to insert the special function for this set
     */
    auto normal_fill = [fid, &front_it, bid]() {
        shared_ptr<string> r = shared_ptr<string>(new string
                                                  (string(SEMANTIC_PROCESS_PREFIX) +=
                                                   std::to_string(fid)));
        
        // register this function
        for (size_t i = fid; i <= bid; ++front_it, ++i)
            front_it->second = r;
    };
    
    // here, to tackle semantic_process, move front_it to the "back id"(bid) position
    if (infobuffer.type == fi_type::sym_left_brace) { // must not use infobuffer.type in the following part
        if (fid != LAST_ID) // for bid does not need to equal to info.ps.size() - 1
            generateException<except::UnexpectedTokenParseException>
            ("Auto-combine production should not specify first kind of semantic process, expect the second kind, expect '='.",
             fi.getOrigin());
        size_t MAX = info.ps.back().first.right.size();
        string &buffer = infobuffer.content;
        buffer = fi.read(object::get_cpp_block); // there should be no comment within it
        
        // if nothing is in, just put empty.
        if (buffer.empty()) {
            generate_empty_semantic_process();
        }
        else {
            std::unordered_set<uint32_t> to_put;
            // to determine which should be put into the block
            for (size_t i = 0; i < buffer.size(); ++i) {
                // $0[number] is out of service range
                if (buffer[i] == '$' && buffer[i + 1] >= '1' && buffer[i + 1] <= '9') {
                    uint32_t r = buffer[i + 1] - '0';
                    size_t k = i + 2;
                    for (; utils::is_number(buffer[k]); ++k)
                        r = r * 10 + buffer[k] - '0';
                    // $0 or $[num] with num > MAX is not in service range
                    if (r > MAX) {
                        if (!info.cmd.no_parse_range_warn)
                            fi.getLogger().warn("$", r, " is out of range.");
                        continue;
                    }
                    to_put.insert(r - 1);
                    // to cope with "++i"
                    i = k - 1;
                }
                // no need to check return, for there may be lambda or other valid "return" inside it.
                //                    else if (buffer[i] == 'r' // check return
                //                             && buffer[i + 1] == 'e'
                //                             && buffer[i + 2] == 't'
                //                             && buffer[i + 3] == 'u'
                //                             && buffer[i + 4] == 'r'
                //                             && buffer[i + 5] == 'n') {
                //                        i += 6;
                //                        while (utils::is_divider(buffer[i])) ++i;
                //                        if (buffer[i] != ';')
                //                            generateException<except::UnexpectedTokenParseException>
                //                                ("return inside C++ semantic process must be \"void\"", fi.getOrigin());
                //                        --i;
                //                    }
            }
            
            // call semantic process in "buffer"
            output_production_semantic_process([&info, &buffer, &fi, &to_put, left, MAX, &normal_fill]() {
                fi.write("auto run = [&info](");
                auto &right = info.ps.back().first.right;
                for (auto idx: to_put) {
                    auto k = right[idx];
#define print_sym_type \
if (k >= 0) fi.write(TOKEN_TYPE);\
else if (k < info.cpp_type_map_min_symbol) fi.write(DEFAULT_OBJECT_TYPE);\
else fi.write(info.nontermiante_cpp_type_map[k].first);
                    
                    print_sym_type
                    
                    fi.write("&$", idx + 1, ", ");
                }
                if (left < info.cpp_type_map_min_symbol) fi.write(DEFAULT_OBJECT_TYPE);
                else fi.write(info.nontermiante_cpp_type_map[left].first);
                fi.writeln("&$$) {");
                fi.writeln(buffer);
                fi.writeln("};");
                fi.write("run(");
                for (auto idx: to_put) {
                    fi.write("std::get<");
                    auto k = right[idx];
                    
                    print_sym_type
                    
                    fi.write(">(right[", idx, "].object), ");
                }
                fi.write("std::get<");
                if (left < info.cpp_type_map_min_symbol) fi.write(DEFAULT_OBJECT_TYPE);
                else fi.write(info.nontermiante_cpp_type_map[left].first);
                fi.writeln(">(left.object));");
#undef print_sym_type
                // This implementation may cause low compile efficiency.
//                fi.write("std::visit(overloaded{[&info](");
//                auto &right = info.ps.back().first.right;
//                for (auto idx: to_put) {
//                    auto k = right[idx];
//                    if (k >= 0) {
//                        fi.write(TOKEN_TYPE);
//                    }
//                    else { // nonterminate symbol
//                        if (k < info.cpp_type_map_min_symbol)
//                            fi.write(DEFAULT_OBJECT_TYPE);
//                        else
//                            fi.write(info.nontermiante_cpp_type_map[k].first);
//                    }
//                    fi.write("&$", idx + 1, ", ");
//                }
//                {
//                    if (left < info.cpp_type_map_min_symbol)
//                        fi.write(DEFAULT_OBJECT_TYPE);
//                    else
//                        fi.write(info.nontermiante_cpp_type_map[left].first);
//                }
//                fi.writeln("&$$) {");
//                fi.writeln(buffer);
//                fi.write("}, [](");
//                for (size_t i = 0; i < to_put.size(); ++i)
//                    fi.write("auto &&, ");
//                fi.writeln("auto &&) { assert(false); }").write("}, ");
//                for (auto idx: to_put)
//                    fi.write("right[", idx, "].object, ");
//                fi.writeln("left.object);");
            }, fid, name_for_left, false);
            
            normal_fill();
        }
        infobuffer.type = fi.read_type(expect<check_match>(object::get_sym, {
            fi_type::sym_or, fi_type::sym_semicolon,
            fi_type::sym_right_brace
        }), not_require);
    } // end '{'
    else if (infobuffer.type == fi_type::sym_equ) { // not '{', but '=' // after this, it must be another production
        if (fi.expect(expect("{"), not_require)) {
            string &buffer = infobuffer.content;
            buffer = fi.read(object::get_cpp_block);
            if (buffer.empty())
                generate_empty_semantic_process();
            else {
                output_production_semantic_process
                    ([&fi, &buffer]() {
                        fi.writeln("auto run = [&left, &right, &info]() {");
                        fi.writeln(buffer);
                        fi.writeln("};");
                        fi.writeln("run();");
                    }, fid, name_for_left, false);
                fi.expect(expect(";"), not_require);
                normal_fill();
            }
        }
        else {
            string buffer = fi.read(object::get_formatted_cpp_code_to_semicolon);
            fi.write(VOID_PREFIX).write(buffer).write(BACK_TYPE_PARA_LIST).writeln(";");
            output_production_semantic_process([&fi, &buffer]() {
                fi.write(buffer).writeln(BACK_CALL);
            }, fid, name_for_left, false);
            
            normal_fill();
        }
        
        infobuffer.type = fi.read_type
        (expect<check_match>(object::get_sym, {
            fi_type::sym_or,
            fi_type::sym_right_brace
        }), not_require);
    }
    else { // no matter what happened, fill the semantic process
        generate_empty_semantic_process();
    }
    
    // here, tackle the auto-generated processes
    {
        block_info &binfo = *info.block_stack.back();
        if (info.cmd.do_batch_for_anonym &&             // first, must allow do_batch_for_anonym
            ((!pl.noback && binfo.withback) ||          // then, with batch back
             (!pl.nofront && binfo.withfront)))         // or with batch front
        {
            size_t k = bid + 1;
            // because for a specific first set, there should only be three kinds of function type:
            // type = INIT_AGAC, LEFT_AGAC, RIGHT_AGAC
            unordered_map<string, shared_ptr<string>> type_map;
            for (; front_it != info.ps.end(); ++front_it) {
                auto &stce = type_map[*front_it->second];
                if (!stce) {
                    output_production_semantic_process([&front_it, &fi] {
                        fi.write(*front_it->second).writeln(FRONT_CALL);
                    }, k, ITER_OBJECT_TYPE, true); // initialize left
                    stce = shared_ptr<string>(new string
                                              (string(SEMANTIC_PROCESS_PREFIX) +=
                                               std::to_string(k)));
                }
                // give the new name for it
                front_it->second = stce;
                ++k;
            }
        }
    }
    
    
    if (infobuffer.type == fi_type::sym_semicolon) {
        infobuffer.type = fi.read_type(expect<check_match>(object::get_sym, {
            fi_type::sym_or,
            fi_type::sym_right_brace
        }), not_require);
    }
    if (infobuffer.type == fi_type::sym_right_brace) {
        fi.getOrigin().unget();
        brk = true;
    }
    if (infobuffer.type == fi_type::finished || infobuffer.type == fi_type::nothing) brk = true; // '|' or finish
    
    
    
    
} while (!brk);
}; // end get_production_set

// analyze annoucement, then returns pair<combination set shared pointer, rank vector shared pointer>
auto announce_analyze =
[&info, &infobuffer, &fi](const fi_type t, bool anonym) [[]] ->
pair<shared_ptr<combination_set>, shared_ptr<rank_vec>> {
    // just check, after it's OK, this line can be removed
    assert(t == fi_type::keyword_left || t == fi_type::keyword_right || t == fi_type::keyword_rank);
    shared_ptr<combination_set> cr;
    shared_ptr<rank_vec> rr;
    
    bool first_finish = false;
    bool last_has_name = false;
    
    fi_type &type = infobuffer.type;
    type = t;
    
    if (type == fi_type::keyword_rank) goto rank_analysis;
    
combination_analysis:
    cr = info.create_combination_set(type == fi_type::keyword_left);
    if (first_finish)
        infobuffer = fi.read_info(expect<check_match>(object::get_identifier, fi_type::identifier), not_require);
    else
        infobuffer = fi.read_info(expect<check_match>(object::get_identifier, {
            fi_type::identifier, fi_type::keyword_rank
        }), not_require);
    if (type == fi_type::identifier) {
        info.check_name(infobuffer.content);
        last_has_name = true;
        if (first_finish) {
            info.name_type_map[std::move(infobuffer.content)] = named_types::mix_property{cr, rr};
            goto ret;
        }
        else {
            info.name_type_map[std::move(infobuffer.content)] = named_types::combination{cr};
            type = fi.read_type(expect(object::get_identifier, fi_type::keyword_rank), not_require);
        }
    }
    if (type == fi_type::keyword_rank) {
        last_has_name = false;
        first_finish = true;
        goto rank_analysis;
    }
    
    goto ret;
rank_analysis:
    rr = info.create_rank_vec();
    if (fi.expect(expect("stable"), not_require)) rr->auto_increase = false;
    if (first_finish)
        infobuffer = fi.read_info(expect<check_match>(object::get_identifier, fi_type::identifier), not_require);
    else
        infobuffer = fi.read_info(expect<check_match>(object::get_identifier, {
            fi_type::identifier, fi_type::keyword_left, fi_type::keyword_right
        }), not_require);
    if (type == fi_type::identifier) {
        info.check_name(infobuffer.content);
        last_has_name = true;
        if (first_finish) {
            info.name_type_map[std::move(infobuffer.content)] = named_types::mix_property{cr, rr};
            goto ret;
        }
        else {
            info.name_type_map[std::move(infobuffer.content)] = named_types::rank{rr};
            infobuffer = fi.read_info(expect<check_match>(object::get_identifier, {
                fi_type::keyword_left, fi_type::keyword_right
            }), not_require);
        }
    }
    if (type == fi_type::keyword_left || type == fi_type::keyword_right) {
        last_has_name = false;
        first_finish = true;
        goto combination_analysis;
    }
    
    
ret:
    if (!(anonym | last_has_name))
        generateException
            <except::NotGetExpectedTokenParseException>
                ("Expect a name in this statement", fi.getOrigin());
    return make_pair(cr, rr);
}; // end annouce_analyze
            
            
            
#pragma message("Start Block Analysis")
            /* Start analysis */
            
            
            
            
            
// fix part analyze
{
    block_info &binfo = *info.block_stack.back();
    auto fill_front_back = [&info, &fi, &binfo, &semantic_fill](bool front) {
        bool with_previous = binfo.previous &&
            (front ? !binfo.plist.nofront && binfo.previous->withfront : !binfo.plist.noback && binfo.previous->withback);
        if (fi.expect(expect(front ? "front" : "back"), not_require)) {
            fi.expect(expect("="));
            if (with_previous)
                semantic_fill(binfo.get_name(), !front, binfo.previous->get_name());
            else
                semantic_fill(binfo.get_name(), !front, nullopt);
            if (front) binfo.withfront = true;
            else binfo.withback = true;
        }
        else if (with_previous) {
            if (front) {
                binfo.withfront = true;
                fi.write(string("void ") += binfo.get_name() += FRONT_TYPE_PARA_LIST).writeln(" {");
                fi.writeln(binfo.previous->get_name() += FRONT_CALL);
                fi.writeln("}");
            }
            else {
				binfo.withback = true;
                fi.write(string("void ") += binfo.get_name() += BACK_TYPE_PARA_LIST).writeln(" {");
                fi.writeln(binfo.previous->get_name() += BACK_CALL);
                fi.writeln("}");
            }
        }
        
    };
    // has property list
    if (fi.expect(expect("properties"), not_require)) {
        fi.expect(expect("="));
        property_list_fill(binfo.plist, binfo.previous ? binfo.previous->this_rank : nullptr);
        fi.expect(expect(";"));
    }
    // has front
    fill_front_back(true);
    // has back
    fill_front_back(false);
}
            
// to fix the problem of meaningless intendent for the sake of coding
do {
    // what can appear here: (lexically)
    // '{', 'left', 'right', 'rank', identifier, 'point', '}'
    // a special kind: finished, check fi.end()
    while (fi.expect(expect(";"), not_require));
    if (fi.end()) goto end_check;
    
    infobuffer = fi.read_info(sequence({
        expect<check_match>(object::get_identifier, {
            fi_type::identifier,
            fi_type::keyword_left,
            fi_type::keyword_right,
            fi_type::keyword_rank,
            fi_type::keyword_point
        }),
        expect<check_match>(object::get_sym, {
            fi_type::sym_left_brace,
            fi_type::sym_right_brace
        })
    }));
    
    fi_type &type = infobuffer.type;
    if (type == fi_type::identifier) {
        // it's a new name:
        // maybe: a nonterminate, a name to:
        // a production, a block or a acproduction
        const auto sit = info.name_type_map.find(infobuffer.content);
        if (sit == info.name_type_map.end()) {
            // move to save the name
            string name = std::move(infobuffer.content);
            infobuffer = fi.read_info(expect(object::get_sym, { fi_type::sym_pro_equ, fi_type::sym_colon }));
            // the next is pro_equ, it's a production, the name is a nonterminate
            // type has the same meaning of infobuffer.type
            if (type == fi_type::sym_pro_equ)
                get_production_set(info.insert_nonterminate(std::move(name)), nullopt);
            else { // the next is ":", it's a name, but not known for what, just keep buffer
                
                // now: <name> ':'
                
                // the next one can be:
                // an identifier: must be a start of a production
                // 'left' / 'right' / 'rank' / '{' / '|'
                infobuffer = fi.read_info(expect(generator::get_identifier(object::get_sym), {
                    fi_type::identifier, fi_type::keyword_left, fi_type::keyword_right,
                    fi_type::keyword_rank, fi_type::sym_left_brace, fi_type::sym_or
                }));
                if (type == fi_type::identifier) { // must be a nonterminate
                    // now: <name> ':' <infobuffer.content> ':='
                    fi.expect(expect(object::get_sym, fi_type::sym_pro_equ));
                    temp_left = info.get_nonterminate(std::move(infobuffer.content));
                    get_production_set(temp_left, std::move(name));
                }
                else if (type == fi_type::sym_or) {
                    if (!temp_left)
                        generateException
                            <except::UnexpectedTokenParseException>("A '|' is not expected here", fi.getOrigin());
                    get_production_set(temp_left, std::move(name));
                }
                else if (type == fi_type::sym_left_brace) { // starts a new block
                    info.create_block(std::move(name), nullptr, nullptr);
                    fi.expect(expect("{"));
                    block_analyze();
                }
                else { // this must be the prefix of a block
                    auto p = announce_analyze(type, false); // this will not change buffer
                    info.create_block(std::move(name), p.first, p.second);
                    fi.expect(expect("{"));
                    block_analyze();
                }
            }
        }
        else { // it's a known name, can only be a production.
            if (sit->second.index() != 1)
                generateException<except::ParseTranslateException>(string("Expected a name of terminate type here."), fi.getOrigin());
            // to get and push a production
            fi.expect(expect<check_match>(object::get_sym, fi_type::sym_pro_equ));
            get_production_set(std::get<named_types::nsymbol>(sit->second).sid, nullopt);
        }
    } // end identifier
    else if (type == fi_type::keyword_point) {      // 'point'
        size_t first_f_id, first_b_id, second_f_id, second_b_id;
        auto get_id = [&buffer, &fi, &info](size_t &fid, size_t &bid) -> bool { // returns if it's reduces
            buffer = fi.read(object::get_unsigned_integer, not_require);
            if (!buffer.empty()) { // int mode
                fid = utils::string2size_t(buffer);
                if (fi.expect(expect("-"), not_require)) {
                    bid = utils::string2size_t(fi.read(object::get_unsigned_integer));
                    if (bid > fid)
                        generateException<except::InvalidIdRangeParseException>("The pointing id range is not valid", fi.getOrigin());
                }
                else bid = fid;
            }
            else { // name mode
                buffer = fi.read(expect<check_match>(object::get_identifier, fi_type::identifier)); // must be identifier
                auto it = info.name_type_map.find(buffer);
                if (it == info.name_type_map.end())
                    generateException<except::UnknownTokenParseException>(string("No such identifier as: ") += buffer, fi.getOrigin());
                visit([&fid, &bid, &it, &fi](auto &&t) {
                    using T = std::decay_t<decltype(t)>;
#define con(type) if constexpr (std::is_same_v<T, std::decay_t<named_types::type>>)
                    con(acproduction) {
                        fid = t.front_id;
                        bid = t.back_id;
                    }
                    else con(block) {
                        if (*t.back_id == -1)
                            generateException<except::CallToNotEndBlockParseException>((string("Block \"") += (it->first)) += "\" is not end", fi.getOrigin());
                        fid = t.front_id;
                        bid = *t.back_id;
                    }
                    else con(production)
                        fid = bid = t.pid;
                    else
                        generateException<except::UnexpectedTokenParseException>((string("Name \"") += (it->first)) += "\" is not of type: production, block or auto-combine production", fi.getOrigin());
#undef con
                }, it->second);
            }
            
            fi.expect(expect(","));
            if (!fi.expect(expect("reduce"), not_require)) {
                fi.expect(expect("shift"));
                return 0;
            }
            return 1;
        }; // end get_id
        uint8_t f = get_id(first_f_id, first_b_id);
        fi.expect(expect(","));
        uint8_t s = get_id(second_f_id, second_b_id);
        fi.expect(expect(";"));
        // to calculate which type it is: f(irst), s(econd)
        // by knowing the true value table
        // the following statement is accquired
        uint8_t r = ((f&s) << 2) | (((f^s) & s) << 1) | ((f^s) & f);
        if (!r) {
            info.logger.log_with_is("Not a valid pointing -- SHIFT SHIFT conflict will not occur", fi.getOrigin());
            continue;
        }
        conflict_type type = conflict_type(r);
        for (size_t f = first_f_id; f <= first_b_id; ++f)
            for (size_t s = second_f_id; s <= second_b_id; ++s)
                info.crtable.set<true>(f, s, type);
    } // end 'point'
    else if (type == fi_type::sym_left_brace) {
        info.create_block(nullopt, nullptr, nullptr);
        block_analyze();
    } // end '{'
    else if (type == fi_type::sym_right_brace) break;
    else { // it's a kind of prefix announcement
        auto p = announce_analyze(type, true);
        if (fi.expect(expect(";"), not_require)) continue;
        // here must be a block
        info.create_block(nullopt, p.first, p.second);
        fi.expect(expect("{"));
        block_analyze();
    }
    
} while (1);
            info.block_stack.pop_back();
            // to return, check something
            if (info.block_stack.empty())
                generateException<except::UnknownTokenParseException>("Unexpected '}'", fi.getOrigin());
            return;
            
            
        end_check:
            info.block_stack.pop_back();
            if (info.block_stack.size())
                generateException
                    <except::UnexpectedEndParseException>
                (string("Lack ") += std::to_string(info.block_stack.size()) += "'}'(s)", fi.getOrigin());
            return;
        };
        
        
        /* Start block analysis */
        info.create_block(nullopt, nullptr, nullptr);
        block_analyze();
        
        
        
        /* Final Generate */
        
        // the map between symbols and result
        info.clean_property();
        const auto type_map = info.generate_sym_list().first;
        const auto cpp_type_map = info.generate_nonterminate_cpp_name_map().first;
        
        shared_ptr<ParseInfo> r(new ParseInfo{
            symbol(info.ter_sym_name_map.size()),           // eof
            info.get_start(),                               // the start symbol of all
            info.get_min_with_cpp(),
            type_map,
            cpp_type_map,
            info.crtable.getCounselTable(),
            info.get_productions_in_vec()
        });
        
        fi.writeln("namespace utils {")
        .writeln("const std::string raw_type_map[] = {");
        for (ll k = r->start; k <= r->eof; ++k)
            fi.writeln(std::quoted(type_map[k], '"'), ", ");
        fi.writeln("};")
        .writeln("const std::string * const type_name_map = raw_type_map + ", -r->start, ";")
        .writeln("}");
        
        fi.writeln("constexpr symbol_type (*semantic_list[])(const ll , std::vector<symbol_type> &, pass_info &) = {");
        for (auto &op_s: info.ps)
            if (op_s.second) fi.writeln(*op_s.second, ", ");
            else throw except::ProductionWithoutProcessInnerException();
        fi.writeln("};");
        
        fi.writeln("constexpr ll eof = ", r->eof, ";");
        
        fi.writeln("constexpr ll left_map[] = {");
        for (const auto &p: r->ps)
            fi.writeln(p.left, ", ");
        fi.writeln("};");
        
        fi.writeln("constexpr size_t pop_size_map[] = {");
        for (const auto &p: r->ps)
            fi.writeln(p.right.size(), ",");
        fi.writeln("};");
        
        
        
#ifdef TEST
        logger.logln("Finish parsing, Results:")
        .logln("Productions:");
        auto print_symbol = [&logger, &type_map](symbol s) -> Logger &{
            if (s >= 0) return logger.log(std::quoted(type_map[s], '\''), " ");
            else return logger.log(type_map[s]).log(" ");
        };
        for (const auto &p: r->ps) {
            logger.log("pid <", p.pid, ">: ");
            print_symbol(p.left).log(":= ");
            for (const auto &s: p.right)
                print_symbol(s);
            logger.logln();
        }
        logger.logln("names:");
        for (symbol s = r->start; s <= r->eof; ++s)
            logger.logln(s, ": ", type_map[s]);
        logger.logln("Other infomation:");
        logger.logln("min_with_cpp: ", r->min_with_cpp);
#endif
        
        return r;
    }
    
}

