/**
 * @file sql_tokenizer.h
 * @brief SQL query tokenizer — splits raw SQL strings into typed token lists.
 *
 * Standalone component with no dependency on schema_parser or query_builder.
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <unordered_set>
#include <ostream>

namespace ml {
namespace sql {
namespace parser {

// -----------------------------------------------------------------------
// Token types
// -----------------------------------------------------------------------
enum class TokenType {
    // Literals
    IDENTIFIER,          // table / column names, aliases
    STRING_LITERAL,      // 'hello'
    NUMERIC_LITERAL,     // 42, 3.14, -1
    BLOB_LITERAL,        // X'1A2B'

    // Keywords (SELECT, INSERT, …)
    KEYWORD,

    // Operators / punctuation
    STAR,                // *
    COMMA,               // ,
    DOT,                 // .
    SEMICOLON,           // ;
    OPEN_PAREN,          // (
    CLOSE_PAREN,         // )
    EQUALS,              // =
    NOT_EQUALS,          // != or <>
    LESS_THAN,           // <
    GREATER_THAN,        // >
    LESS_EQUAL,          // <=
    GREATER_EQUAL,       // >=
    PLUS,                // +
    MINUS,               // -
    SLASH,               // /
    PERCENT,             // %
    PIPE,                // ||  (string concatenation in SQL)
    AMPERSAND,           // &
    TILDE,               // ~
    EXCLAMATION,         // !

    // Bind parameters
    PLACEHOLDER,         // ? or :name or $1

    // Comments
    LINE_COMMENT,        // -- …
    BLOCK_COMMENT,       // /* … */

    // Whitespace (optionally preserved)
    WHITESPACE,

    // End of input
    END_OF_INPUT,

    // Anything the tokenizer cannot classify
    UNKNOWN
};

// Human-readable name for a token type.
const char* token_type_name(TokenType type) noexcept;

// -----------------------------------------------------------------------
// Token
// -----------------------------------------------------------------------
struct Token {
    TokenType   type   = TokenType::UNKNOWN;
    std::string value;           // original lexeme text
    std::size_t line   = 1;      // 1-based line number
    std::size_t column = 1;      // 1-based column number

    bool is_keyword() const noexcept { return type == TokenType::KEYWORD; }
    bool is_identifier() const noexcept { return type == TokenType::IDENTIFIER; }
    bool is_literal() const noexcept;

    bool operator==(const Token& o) const noexcept {
        return type == o.type && value == o.value;
    }
    bool operator!=(const Token& o) const noexcept { return !(*this == o); }
};

std::ostream& operator<<(std::ostream& os, const Token& token);

// -----------------------------------------------------------------------
// Tokenizer configuration
// -----------------------------------------------------------------------
struct TokenizerConfig {
    bool preserve_whitespace = false;   // emit WHITESPACE tokens?
    bool preserve_comments   = false;   // emit LINE_COMMENT / BLOCK_COMMENT?
    bool uppercase_keywords  = true;    // normalise keyword casing to upper?
};

// -----------------------------------------------------------------------
// SQLTokenizer
// -----------------------------------------------------------------------
class SQLTokenizer {
public:
    explicit SQLTokenizer(const TokenizerConfig& config = {});

    /// Tokenize a full SQL string (may contain multiple statements).
    std::vector<Token> tokenize(const std::string& sql) const;

    /// Convenience: tokenize and return only non-whitespace, non-comment tokens.
    std::vector<Token> tokenize_stripped(const std::string& sql) const;

    /// Access the keyword set (read-only).
    const std::unordered_set<std::string>& keywords() const noexcept;

private:
    TokenizerConfig config_;
    std::unordered_set<std::string> keywords_;

    void init_keywords();

    // ---- low-level scanning helpers (operate on position state) ----------
    struct Cursor {
        const char* start  = nullptr;
        const char* end    = nullptr;
        const char* pos    = nullptr;
        std::size_t line   = 1;
        std::size_t column = 1;

        char peek() const noexcept;
        char peek_next() const noexcept;
        char advance() noexcept;
        bool at_end() const noexcept;
    };

    Token scan_token(Cursor& cur) const;
    Token scan_string(Cursor& cur) const;
    Token scan_number(Cursor& cur) const;
    Token scan_word(Cursor& cur) const;
    Token scan_line_comment(Cursor& cur) const;
    Token scan_block_comment(Cursor& cur) const;
    Token scan_placeholder(Cursor& cur) const;
    Token scan_whitespace(Cursor& cur) const;

    static std::string to_upper(const std::string& s);
};

} // namespace parser
} // namespace sql
} // namespace ml
