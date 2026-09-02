#include "../headers/lexer_js.h"
#include <cctype>
#include <vector>

namespace pljs {

LexerJS::LexerJS(const std::string &src): src_(src) {}

char LexerJS::peek() const { return i_ < src_.size() ? src_[i_] : '\0'; }
char LexerJS::get() { return i_ < src_.size() ? src_[i_++] : '\0'; }

void LexerJS::skip_space() {
    while(true) {
        char c = peek();
        if(c=='\r') { get(); continue; }
        if(c==' '||c=='\t') { get(); continue; }
        if(c=='/' && i_+1<src_.size() && src_[i_+1]=='/') { // line comment
            get(); get();
            while(peek()!='\n' && peek()!='\0') get();
            continue;
        }
        break;
    }
}

bool LexerJS::is_ident_start(char c) const { return std::isalpha((unsigned char)c) || c=='_' || c=='$'; }
bool LexerJS::is_ident_char(char c) const { return std::isalnum((unsigned char)c) || c=='_' || c=='$'; }

std::vector<TokenJS> LexerJS::tokenize() {
    std::vector<TokenJS> out;
    while(true) {
        skip_space();
        char c = peek();
        if(c=='\0') break;
        if(c=='\n') { get(); out.push_back({TokenTypeJS::Symbol,"\n",line_}); ++line_; continue; }
        if(std::isdigit((unsigned char)c)) {
            std::string num;
            while(std::isdigit((unsigned char)peek())||peek()=='.') num.push_back(get());
            out.push_back({TokenTypeJS::Number,num,line_});
            continue;
        }
        if(is_ident_start(c)) {
            std::string id;
            while(is_ident_char(peek())) id.push_back(get());
            static const std::vector<std::string> kw={"function","var","let","const","if","else","return"};
            bool iskw=false;
            for(auto &k:kw) if(k==id) { iskw=true; break; }
            out.push_back({iskw?TokenTypeJS::Keyword:TokenTypeJS::Identifier,id,line_});
            continue;
        }
        if(c=='"' || c=='\'') {
            char delim = get();
            std::string s;
            while(peek()!=delim && peek()!='\0') {
                if(peek()=='\\') { s.push_back(get()); if(peek()!= '\0') s.push_back(get()); continue; }
                s.push_back(get());
            }
            if(peek()==delim) get();
            out.push_back({TokenTypeJS::String,s,line_});
            continue;
        }
        // symbols/operators
        std::string sym;
        sym.push_back(get());
        out.push_back({TokenTypeJS::Symbol,sym,line_});
    }
    out.push_back({TokenTypeJS::End,"",line_});
    return out;
}

} // namespace pljs
