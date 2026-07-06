#pragma once
#include "lexer_js.h"
#include <memory>
#include <vector>

namespace pljs {

class ParserJS {
public:
    explicit ParserJS(const std::vector<TokenJS>& tokens);
    // parse returns true on success, throws std::runtime_error on failure
    bool parse();
private:
    const std::vector<TokenJS> tokens_;
    size_t i_ = 0;
    const TokenJS& peek() const;
    const TokenJS& get();
    void parse_program();
    void parse_statement();
    void parse_function();
    void parse_var_decl();
};

} // namespace pljs
