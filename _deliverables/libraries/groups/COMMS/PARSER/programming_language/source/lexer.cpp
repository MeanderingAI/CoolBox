#include "../headers/lexer.h"
#include <cctype>

namespace plang {

Lexer::Lexer(const std::string &src): src_(src) {}

char Lexer::peek() const { return i_ < src_.size() ? src_[i_] : '\0'; }
char Lexer::get() { return i_ < src_.size() ? src_[i_++] : '\0'; }

void Lexer::skip_space() {
    while(true) {
        char c = peek();
        if(c=='\r') { get(); continue; }
        if(c==' '||c=='\t') { get(); continue; }
        if(c=='%') { // comment to end of line
            while(peek()!='\n' && peek()!='\0') get();
            continue;
        }
        break;
    }
}

bool Lexer::is_ident_start(char c) const { return std::isalpha((unsigned char)c) || c=='_'; }
bool Lexer::is_ident_char(char c) const { return std::isalnum((unsigned char)c) || c=='_'; }

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    while(true) {
        skip_space();
        char c = peek();
        if(c=='\0') break;
        if(c=='\n') { get(); out.push_back({TokenType::Newline,"\n",line_}); ++line_; continue; }
        if(std::isdigit((unsigned char)c)) {
            std::string num;
            while(std::isdigit((unsigned char)peek())||peek()=='.') num.push_back(get());
            out.push_back({TokenType::Number,num,line_});
            continue;
        }
        if(is_ident_start(c)) {
            std::string id;
            while(is_ident_char(peek())) id.push_back(get());
            static const std::vector<std::string> kw={"function","end","if","else","for","while","return"};
            bool iskw=false;
            for(auto &k:kw) if(k==id) { iskw=true; break; }
            out.push_back({iskw?TokenType::Keyword:TokenType::Identifier,id,line_});
            continue;
        }
        // symbols and operators
        std::string s;
        s.push_back(get());
        out.push_back({TokenType::Symbol,s,line_});
    }
    out.push_back({TokenType::End,"",line_});
    return out;
}

} // namespace plang
