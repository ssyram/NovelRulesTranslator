# NovelRulesTranslator
The reformed version of RulesTranslator

This project is a personal project aming to practice my skills in making compiler as well as writing C++ codes. It's a reformed version of Rules Translator.

This project aims at making a simple compiler to translate a simple language that is written accompanied with C++ into C++ source code.

## Requirement
Compiler support C++17 standard as well as C++17 standard library.

## Usage
#### Simple Usage: <br />
> ./rtsl \<source path\> [[-o] \<target path\>]

There must be a source path that ended with ".tsl", if not specified, \<target path\> would be souce path with postfix of ".cpp"
#### Full Usage:
> ./rtsl [property_list] \<souce path\> [property_list]<br/>
##### property_list:

[-o] \<output file path\> \*:default is the same as source file with postfix .cpp<br />
-thread \<unsigned int to specify thread amount\> \*: [1 - 100]<br />
-no_parse_range_warn <br />
-restrict_nonformat_terminate<br />
-ignore_duplicate_production \*: just for auto-combine ones<br />
-ignore_anonym_for_cr<br />
-property_list_for_auto_generate<br />
-ignore_batch_for_anonym<br />
-show_all_collisions
