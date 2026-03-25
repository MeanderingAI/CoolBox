#include "../headers/parser_js.h"
#include <stdexcept>

namespace pljs {

ParserJS::ParserJS(const std::vector<TokenJS>& tokens): tokens_(tokens) {}

const TokenJS& ParserJS::peek() const { return tokens_[i_]; }
const TokenJS& ParserJS::get() { return tokens_[i_++]; }

bool ParserJS::parse() {
    try {
        parse_program();
        return true;
    } catch(const std::exception &ex) {
        throw;
    }
}

void ParserJS::parse_program() {
    while(peek().type!=TokenTypeJS::End) {
        parse_statement();
        if(peek().type==TokenTypeJS::Symbol && peek().text==";") get();
        else if(peek().type==TokenTypeJS::Symbol && peek().text=="\n") get();
        else {
            // allow missing semicolon
        }
    }
}

void ParserJS::parse_statement() {
    if(peek().type==TokenTypeJS::Keyword && (peek().text=="function")) { parse_function(); return; }
    if(peek().type==TokenTypeJS::Keyword && (peek().text=="var"||peek().text=="let"||peek().text=="const")) { parse_var_decl(); return; }
    // fallback: consume token
    if(peek().type==TokenTypeJS::End) return;
    get();
}

void ParserJS::parse_function() {
    // function <name>(args) { ... }
    get(); // function
    if(peek().type!=TokenTypeJS::Identifier) throw std::runtime_error("Expected function name");
    get(); // name
    if(!(peek().type==TokenTypeJS::Symbol && peek().text=="(")) throw std::runtime_error("Expected '('");
    get(); // (
    // skip params
    while(!(peek().type==TokenTypeJS::Symbol && peek().text==")") && peek().type!=TokenTypeJS::End) { get(); }
    if(peek().type==TokenTypeJS::Symbol && peek().text==")") get();
    if(!(peek().type==TokenTypeJS::Symbol && peek().text=="{")) throw std::runtime_error("Expected '{'");
    // skip body until matching '}' (naive)
    int depth=0;
    while(peek().type!=TokenTypeJS::End) {
        if(peek().type==TokenTypeJS::Symbol && peek().text=="{") { depth++; get(); continue; }
        if(peek().type==TokenTypeJS::Symbol && peek().text=="}") { get(); if(depth<=0) break; depth--; continue; }
        get();
    }
}

void ParserJS::parse_var_decl() {
    // var/let/const name [= expr]
    get(); // var/let/const
    if(peek().type!=TokenTypeJS::Identifier) throw std::runtime_error("Expected variable name");
    get();
    if(peek().type==TokenTypeJS::Symbol && peek().text=="=") {
        get(); // =
        // skip simple expression tokens until semicolon or newline
        while(peek().type!=TokenTypeJS::End && !(peek().type==TokenTypeJS::Symbol && (peek().text==";"||peek().text=="\n"))) get();
    }
}

} // namespace pljs
