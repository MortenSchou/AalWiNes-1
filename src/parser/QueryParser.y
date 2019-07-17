%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.0"
%defines
%define api.namespace {mpls2pda}
%define parser_class_name {Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    /* 
     * This program is free software: you can redistribute it and/or modify
     * it under the terms of the GNU General Public License as published by
     * the Free Software Foundation, either version 3 of the License, or
     * (at your option) any later version.
     * 
     * This program is distributed in the hope that it will be useful,
     * but WITHOUT ANY WARRANTY; without even the implied warranty of
     * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     * GNU General Public License for more details.
     * 
     * You should have received a copy of the GNU General Public License
     * along with this program.  If not, see <http://www.gnu.org/licenses/>.
     */
    //
    // Created by Peter G. Jensen on 10/15/18.
    //
    #include <string>
    #include <unordered_map>
    #include <memory>
    #include "pdaaal/model/NFA.h"
    #include "model/Query.h"

    namespace mpls2pda {
        class Builder;
        class Scanner;
    }
    
    using namespace pdaaal;
    using namespace mpls2pda;
}


%parse-param { Scanner& scanner  }
%parse-param { Builder& builder }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include "QueryBuilder.h"
    #include "Scanner.h"
    #include "model/Query.h"
    #include "pdaaal/model/NFA.h"

    using namespace pdaaal;
    using namespace mpls2pda;

    #undef yylex
    #define yylex ([&](){\
        return Parser::symbol_type((Parser::token_type)scanner.yylex(), builder._location);\
        })
}

%token
        END  0    "end of file"
        LPAREN    "("
        RPAREN    ")"
        
        AND       "&"
        OR        "|"
        DOT       "."
        PLUS      "+"

        LSQBRCKT  "["
        RSQBRCKT  "]"
        LT        "<"
        GT        ">"       
        
        IDENTIFIER  "identifier"
        LITERAL     "literal"        
        NUMBER     "number"
        HEX        "hex"
        
        COMMA     ","
        HAT       "^"
        HASH      "#"
        COLON     ":"
;


// bison does not seem to like naked shared pointers :(
%type  <int64_t> number hexnumber;
%type  <std::vector<Query>> query_list;
%type  <Query> query;
%type  <NFA<Query::label_t>> regex;
%type  <Query::mode_t> mode;
%type  <std::vector<Query::label_t>> atom atom_list ip4 ip6 identifier label name;
%type  <std::string> literal;
//%printer { yyoutput << $$; } <*>;
%left AND OR DOT PLUS STAR
%%
%start query_list;
query_list
        : query_list query { std::swap($$,$1); $$.emplace_back(std::move($2)); }
        | query { $$ = {std::move($1)}; }
        | { $$ = {};  }// empty 
        ;
query
    : LT { builder.link_mode(); } regex 
      GT { builder.path_mode(); } regex 
      LT { builder.link_mode(); } regex GT number mode
    {
        $$ = Query(std::move($3), std::move($6), std::move($9), $11, $12);
    }
    ;

mode 
    : "OVER" { $$ = Query::OVER; }
    | "UNDER" { $$ = Query::UNDER; }
    | "EXACT" { $$ = Query::EXACT; }
    | { $$ = Query::OVER; } //empty 
    ;

regex
    : LSQBRCKT atom_list RSQBRCKT { $$ = NFA<Query::label_t>(std::move($2), false); }
    | LSQBRCKT HAT atom_list RSQBRCKT { $$ = NFA<Query::label_t>(std::move($3), true); } // negated set
    | LPAREN regex RPAREN { $$ = std::move($2); }
    | regex AND regex { $$ = std::move($1); $$.and_extend($3); }
    | regex OR regex { $$ = std::move($1); $$.or_extend($3); }
    | regex DOT regex { $$ = std::move($1); $$.dot_extend($3); }
    | regex PLUS regex { $$ = std::move($1); $$.plus_extend($3); }
    | regex STAR regex { $$ = std::move($1); $$.star_extend($3); }
    | { $$ = NFA<Query::label_t>(); } // empty.
    ;
    
number
    : NUMBER {
        $$ = scanner.last_int;
    }
    ;
    
hexnumber
    : HEX {
        $$ = scanner.last_int;
    }
    ;
    
atom_list
    : atom COMMA atom_list { $$ = std::move($3); $$.insert($$.end(), $1.begin(), $1.end()); }
    | atom { $$ = std::move($1); }
    ;
    
atom 
    : label { $$ = std::move($1); }     // labels
    | identifier HASH { builder.set_filter(std::move($1), false); }
      identifier { $$ = std::move($4); builder.clear_filter(); }// LINKS
    | HASH identifier { $$ = std::move($2); } // ingoing
    | identifier HASH { $$ = std::move($1); } // outgoing
    ;

identifier
    : name { $$ = std::move($1); }
    | name DOT { builder.set_filter(std::move($1), true);} name
    { $$ = std::move($4); builder.clear_filter(); }
    | "_" { $$ = builder.ip_id(); }
    | "^" { $$ = builder.routing_id(); }
    | "!" { $$ = builder.discard_id(); }
    ;

ip4
    : number DOT number DOT number DOT number "/" number { 
        $$ = builder.match_ip4($1, $3, $5, $7, $9);
    }
    | number DOT number DOT number DOT number {
        $$ = builder.match_ip4($1, $3, $5, $7, 1);
    }
    ;

ip6
    : hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber "/" number {
        $$ = builder.match_ip6($1, $3, $5, $7, $9, $11, $13);
    }
    | hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber COLON hexnumber { 
        $$ = builder.match_ip6($1, $3, $5, $7, $9, $11, $13);
    }
    ;

literal
    : LITERAL {
        $$ = scanner.last_string.substr(1, scanner.last_string.length()-2) ;
    }
    ;

    
name 
    : ip4 { $$ = std::move($1); }
    | ip6 { $$ = std::move($1); }
    | IDENTIFIER { $$ = builder.match_exact(scanner.last_string); }
    | literal {  $$ = builder.match_re(std::move($1)); }
    ;

label
    : number  { $$ = builder.find_label($1, 1); }
    | number "/" number  { $$ = builder.find_label($1, $3); }
    ;
    

%%

void
mpls2pda::Parser::error (const location_type& l,
                          const std::string& m)
{
  builder.error(l, m);
}