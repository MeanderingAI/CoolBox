/**
 * @file sql_tokenizer.cpp
 * @brief Implementation of the SQL tokenizer.
 */

#include "../headers/sql_tokenizer.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ml {
namespace sql {
namespace parser {

// -----------------------------------------------------------------------
// token_type_name
// -----------------------------------------------------------------------
const char* token_type_name(TokenType type) noexcept {
    switch (type) {
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::STRING_LITERAL:  return "STRING_LITERAL";
        case TokenType::NUMERIC_LITERAL: return "NUMERIC_LITERAL";
        case TokenType::BLOB_LITERAL:    return "BLOB_LITERAL";
        case TokenType::KEYWORD:         return "KEYWORD";
        case TokenType::STAR:            return "STAR";
        case TokenType::COMMA:           return "COMMA";
        case TokenType::DOT:             return "DOT";
        case TokenType::SEMICOLON:       return "SEMICOLON";
        case TokenType::OPEN_PAREN:      return "OPEN_PAREN";
        case TokenType::CLOSE_PAREN:     return "CLOSE_PAREN";
        case TokenType::EQUALS:          return "EQUALS";
        case TokenType::NOT_EQUALS:      return "NOT_EQUALS";
        case TokenType::LESS_THAN:       return "LESS_THAN";
        case TokenType::GREATER_THAN:    return "GREATER_THAN";
        case TokenType::LESS_EQUAL:      return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL:   return "GREATER_EQUAL";
        case TokenType::PLUS:            return "PLUS";
        case TokenType::MINUS:           return "MINUS";
        case TokenType::SLASH:           return "SLASH";
        case TokenType::PERCENT:         return "PERCENT";
        case TokenType::PIPE:            return "PIPE";
        case TokenType::AMPERSAND:       return "AMPERSAND";
        case TokenType::TILDE:           return "TILDE";
        case TokenType::EXCLAMATION:     return "EXCLAMATION";
        case TokenType::PLACEHOLDER:     return "PLACEHOLDER";
        case TokenType::LINE_COMMENT:    return "LINE_COMMENT";
        case TokenType::BLOCK_COMMENT:   return "BLOCK_COMMENT";
        case TokenType::WHITESPACE:      return "WHITESPACE";
        case TokenType::END_OF_INPUT:    return "END_OF_INPUT";
        case TokenType::UNKNOWN:         return "UNKNOWN";
    }
    return "UNKNOWN";
}

// -----------------------------------------------------------------------
// Token helpers
// -----------------------------------------------------------------------
bool Token::is_literal() const noexcept {
    return type == TokenType::STRING_LITERAL ||
           type == TokenType::NUMERIC_LITERAL ||
           type == TokenType::BLOB_LITERAL;
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "[" << token_type_name(token.type)
       << " \"" << token.value << "\""
       << " L" << token.line << ":" << token.column << "]";
    return os;
}

// -----------------------------------------------------------------------
// Cursor
// -----------------------------------------------------------------------
char SQLTokenizer::Cursor::peek() const noexcept {
    return at_end() ? '\0' : *pos;
}

char SQLTokenizer::Cursor::peek_next() const noexcept {
    if (pos + 1 >= end) return '\0';
    return *(pos + 1);
}

char SQLTokenizer::Cursor::advance() noexcept {
    if (at_end()) return '\0';
    char c = *pos++;
    if (c == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    return c;
}

bool SQLTokenizer::Cursor::at_end() const noexcept {
    return pos >= end;
}

// -----------------------------------------------------------------------
// SQLTokenizer construction
// -----------------------------------------------------------------------
SQLTokenizer::SQLTokenizer(const TokenizerConfig& config)
    : config_(config) {
    init_keywords();
}

void SQLTokenizer::init_keywords() {
    // SQL-92 / SQL-99 common keywords + widely-used extensions.
    static const char* kw[] = {
        "ABORT", "ACTION", "ADD", "AFTER", "ALL", "ALTER", "ALWAYS",
        "ANALYZE", "AND", "AS", "ASC",
        "AUTOINCREMENT", "BEFORE", "BEGIN", "BETWEEN", "BY",
        "CASCADE", "CASE", "CAST", "CHECK", "COLLATE", "COLUMN",
        "COMMIT", "CONFLICT", "CONSTRAINT", "COUNT", "CREATE", "CROSS",
        "CURRENT", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP",
        "DATABASE", "DEFAULT", "DEFERRABLE", "DEFERRED", "DELETE",
        "DESC", "DETACH", "DISTINCT", "DO", "DROP",
        "EACH", "ELSE", "END", "ESCAPE", "EXCEPT", "EXCLUDE",
        "EXCLUSIVE", "EXISTS", "EXPLAIN", "FAIL", "FILTER",
        "FIRST", "FOLLOWING", "FOR", "FOREIGN", "FROM", "FULL",
        "GENERATED", "GLOB", "GROUP", "GROUPS",
        "HAVING", "IF", "IGNORE", "IMMEDIATE", "IN", "INDEX",
        "INDEXED", "INITIALLY", "INNER", "INSERT", "INSTEAD",
        "INTERSECT", "INTO", "IS", "ISNULL",
        "JOIN", "KEY",
        "LAST", "LEFT", "LIKE", "LIMIT",
        "MATCH", "MATERIALIZED", "MAX", "MIN",
        "NATURAL", "NO", "NOT", "NOTHING", "NOTNULL", "NULL", "NULLS",
        "OF", "OFFSET", "ON", "OR", "ORDER", "OTHERS", "OUTER", "OVER",
        "PARTITION", "PLAN", "PRAGMA", "PRECEDING", "PRIMARY",
        "QUERY",
        "RAISE", "RANGE", "RECURSIVE", "REFERENCES", "REGEXP",
        "REINDEX", "RELEASE", "RENAME", "REPLACE", "RESTRICT",
        "RETURNING", "RIGHT", "ROLLBACK", "ROW", "ROWS",
        "SAVEPOINT", "SELECT", "SET", "SUM",
        "TABLE", "TEMP", "TEMPORARY", "THEN", "TIES", "TO",
        "TRANSACTION", "TRIGGER", "TRUNCATE",
        "UNBOUNDED", "UNION", "UNIQUE", "UPDATE", "USING",
        "VACUUM", "VALUES", "VIEW", "VIRTUAL",
        "WHEN", "WHERE", "WINDOW", "WITH", "WITHOUT",
        // Data types often treated as keywords
        "INT", "INTEGER", "REAL", "TEXT", "BLOB", "VARCHAR",
        "CHAR", "BOOLEAN", "DATE", "DATETIME", "TIMESTAMP",
        "FLOAT", "DOUBLE", "DECIMAL", "NUMERIC", "BIGINT",
        "SMALLINT", "TINYINT", "SERIAL", "BIGSERIAL",
        // Boolean literals
        "TRUE", "FALSE",
        // Aggregates / functions sometimes tokenized as keywords
        "AVG", "GROUP_CONCAT", "TOTAL", "ABS", "COALESCE",
        "IFNULL", "LENGTH", "LOWER", "UPPER", "SUBSTR", "TRIM",
        "TYPEOF", "RANDOM", "ROUND",
    };

    for (const char* k : kw) {
        keywords_.emplace(k);
    }
}

const std::unordered_set<std::string>& SQLTokenizer::keywords() const noexcept {
    return keywords_;
}

// -----------------------------------------------------------------------
// to_upper
// -----------------------------------------------------------------------
std::string SQLTokenizer::to_upper(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return out;
}

// -----------------------------------------------------------------------
// tokenize
// -----------------------------------------------------------------------
std::vector<Token> SQLTokenizer::tokenize(const std::string& sql) const {
    Cursor cur;
    cur.start  = sql.data();
    cur.end    = sql.data() + sql.size();
    cur.pos    = cur.start;
    cur.line   = 1;
    cur.column = 1;

    std::vector<Token> tokens;
    tokens.reserve(sql.size() / 4);  // rough estimate

    while (!cur.at_end()) {
        Token tok = scan_token(cur);

        if (tok.type == TokenType::WHITESPACE && !config_.preserve_whitespace) {
            continue;
        }
        if ((tok.type == TokenType::LINE_COMMENT ||
             tok.type == TokenType::BLOCK_COMMENT) &&
            !config_.preserve_comments) {
            continue;
        }

        tokens.push_back(std::move(tok));
    }

    // Append sentinel
    Token eof;
    eof.type   = TokenType::END_OF_INPUT;
    eof.line   = cur.line;
    eof.column = cur.column;
    tokens.push_back(eof);

    return tokens;
}

std::vector<Token> SQLTokenizer::tokenize_stripped(const std::string& sql) const {
    // Force stripping regardless of config
    TokenizerConfig stripped_cfg = config_;
    stripped_cfg.preserve_whitespace = false;
    stripped_cfg.preserve_comments   = false;

    SQLTokenizer tmp(stripped_cfg);
    return tmp.tokenize(sql);
}

// -----------------------------------------------------------------------
// scan_token — main dispatch
// -----------------------------------------------------------------------
Token SQLTokenizer::scan_token(Cursor& cur) const {
    char c = cur.peek();

    // Whitespace
    if (std::isspace(static_cast<unsigned char>(c))) {
        return scan_whitespace(cur);
    }

    // Line comment: -- ...
    if (c == '-' && cur.peek_next() == '-') {
        return scan_line_comment(cur);
    }

    // Block comment: /* ... */
    if (c == '/' && cur.peek_next() == '*') {
        return scan_block_comment(cur);
    }

    // String literal: 'text' (SQL standard) or "text" (identifier quoting)
    if (c == '\'') {
        return scan_string(cur);
    }

    // Double-quoted identifier
    if (c == '"') {
        Token tok;
        tok.line   = cur.line;
        tok.column = cur.column;
        cur.advance(); // skip opening "
        std::string val;
        while (!cur.at_end() && cur.peek() != '"') {
            val += cur.advance();
        }
        if (!cur.at_end()) cur.advance(); // skip closing "
        tok.type  = TokenType::IDENTIFIER;
        tok.value = val;
        return tok;
    }

    // Backtick-quoted identifier (MySQL style)
    if (c == '`') {
        Token tok;
        tok.line   = cur.line;
        tok.column = cur.column;
        cur.advance(); // skip opening `
        std::string val;
        while (!cur.at_end() && cur.peek() != '`') {
            val += cur.advance();
        }
        if (!cur.at_end()) cur.advance(); // skip closing `
        tok.type  = TokenType::IDENTIFIER;
        tok.value = val;
        return tok;
    }

    // Blob literal: X'...' or x'...'
    if ((c == 'X' || c == 'x') && cur.peek_next() == '\'') {
        Token tok;
        tok.line   = cur.line;
        tok.column = cur.column;
        std::string val;
        val += cur.advance(); // X
        val += cur.advance(); // '
        while (!cur.at_end() && cur.peek() != '\'') {
            val += cur.advance();
        }
        if (!cur.at_end()) val += cur.advance(); // closing '
        tok.type  = TokenType::BLOB_LITERAL;
        tok.value = val;
        return tok;
    }

    // Numeric literal  (integers, decimals, hex 0x…)
    if (std::isdigit(static_cast<unsigned char>(c)) ||
        (c == '.' && std::isdigit(static_cast<unsigned char>(cur.peek_next())))) {
        return scan_number(cur);
    }

    // Word (identifier or keyword)
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return scan_word(cur);
    }

    // Placeholder: ? or :name or $n
    if (c == '?' || c == ':' || c == '$') {
        return scan_placeholder(cur);
    }

    // Single / multi-character operators & punctuation
    Token tok;
    tok.line   = cur.line;
    tok.column = cur.column;

    switch (c) {
        case '*': tok.type = TokenType::STAR;        tok.value = "*"; cur.advance(); return tok;
        case ',': tok.type = TokenType::COMMA;       tok.value = ","; cur.advance(); return tok;
        case '.': tok.type = TokenType::DOT;         tok.value = "."; cur.advance(); return tok;
        case ';': tok.type = TokenType::SEMICOLON;   tok.value = ";"; cur.advance(); return tok;
        case '(': tok.type = TokenType::OPEN_PAREN;  tok.value = "("; cur.advance(); return tok;
        case ')': tok.type = TokenType::CLOSE_PAREN; tok.value = ")"; cur.advance(); return tok;
        case '+': tok.type = TokenType::PLUS;        tok.value = "+"; cur.advance(); return tok;
        case '-': tok.type = TokenType::MINUS;       tok.value = "-"; cur.advance(); return tok;
        case '/': tok.type = TokenType::SLASH;       tok.value = "/"; cur.advance(); return tok;
        case '%': tok.type = TokenType::PERCENT;     tok.value = "%"; cur.advance(); return tok;
        case '~': tok.type = TokenType::TILDE;       tok.value = "~"; cur.advance(); return tok;
        case '&': tok.type = TokenType::AMPERSAND;   tok.value = "&"; cur.advance(); return tok;

        case '|':
            cur.advance();
            if (cur.peek() == '|') {
                cur.advance();
                tok.type  = TokenType::PIPE;
                tok.value = "||";
            } else {
                tok.type  = TokenType::PIPE;
                tok.value = "|";
            }
            return tok;

        case '=':
            cur.advance();
            tok.type  = TokenType::EQUALS;
            tok.value = "=";
            return tok;

        case '!':
            cur.advance();
            if (cur.peek() == '=') {
                cur.advance();
                tok.type  = TokenType::NOT_EQUALS;
                tok.value = "!=";
            } else {
                tok.type  = TokenType::EXCLAMATION;
                tok.value = "!";
            }
            return tok;

        case '<':
            cur.advance();
            if (cur.peek() == '=') {
                cur.advance();
                tok.type  = TokenType::LESS_EQUAL;
                tok.value = "<=";
            } else if (cur.peek() == '>') {
                cur.advance();
                tok.type  = TokenType::NOT_EQUALS;
                tok.value = "<>";
            } else {
                tok.type  = TokenType::LESS_THAN;
                tok.value = "<";
            }
            return tok;

        case '>':
            cur.advance();
            if (cur.peek() == '=') {
                cur.advance();
                tok.type  = TokenType::GREATER_EQUAL;
                tok.value = ">=";
            } else {
                tok.type  = TokenType::GREATER_THAN;
                tok.value = ">";
            }
            return tok;

        default:
            tok.type  = TokenType::UNKNOWN;
            tok.value = std::string(1, cur.advance());
            return tok;
    }
}

// -----------------------------------------------------------------------
// scan_whitespace
// -----------------------------------------------------------------------
Token SQLTokenizer::scan_whitespace(Cursor& cur) const {
    Token tok;
    tok.type   = TokenType::WHITESPACE;
    tok.line   = cur.line;
    tok.column = cur.column;

    std::string val;
    while (!cur.at_end() && std::isspace(static_cast<unsigned char>(cur.peek()))) {
        val += cur.advance();
    }
    tok.value = val;
    return tok;
}

// -----------------------------------------------------------------------
// scan_string  —  single-quoted string with '' escape
// -----------------------------------------------------------------------
Token SQLTokenizer::scan_string(Cursor& cur) const {
    Token tok;
    tok.type   = TokenType::STRING_LITERAL;
    tok.line   = cur.line;
    tok.column = cur.column;

    cur.advance(); // skip opening quote
    std::string val;
    while (!cur.at_end()) {
        char c = cur.advance();
        if (c == '\'') {
            // Escaped quote '' ?
            if (cur.peek() == '\'') {
                val += '\'';
                cur.advance();
            } else {
                break; // end of string
            }
        } else {
            val += c;
        }
    }
    tok.value = val;
    return tok;
}

// -----------------------------------------------------------------------
// scan_number
// -----------------------------------------------------------------------
Token SQLTokenizer::scan_number(Cursor& cur) const {
    Token tok;
    tok.type   = TokenType::NUMERIC_LITERAL;
    tok.line   = cur.line;
    tok.column = cur.column;

    std::string val;

    // Hex literal: 0x…
    if (cur.peek() == '0' && (cur.peek_next() == 'x' || cur.peek_next() == 'X')) {
        val += cur.advance(); // 0
        val += cur.advance(); // x
        while (!cur.at_end() && std::isxdigit(static_cast<unsigned char>(cur.peek()))) {
            val += cur.advance();
        }
        tok.value = val;
        return tok;
    }

    // Integer / decimal part
    while (!cur.at_end() && std::isdigit(static_cast<unsigned char>(cur.peek()))) {
        val += cur.advance();
    }

    // Fractional part
    if (cur.peek() == '.' && std::isdigit(static_cast<unsigned char>(cur.peek_next()))) {
        val += cur.advance(); // .
        while (!cur.at_end() && std::isdigit(static_cast<unsigned char>(cur.peek()))) {
            val += cur.advance();
        }
    } else if (cur.peek() == '.' && val.empty()) {
        // leading dot: .5
        val += cur.advance(); // .
        while (!cur.at_end() && std::isdigit(static_cast<unsigned char>(cur.peek()))) {
            val += cur.advance();
        }
    }

    // Exponent part
    if (cur.peek() == 'e' || cur.peek() == 'E') {
        val += cur.advance();
        if (cur.peek() == '+' || cur.peek() == '-') {
            val += cur.advance();
        }
        while (!cur.at_end() && std::isdigit(static_cast<unsigned char>(cur.peek()))) {
            val += cur.advance();
        }
    }

    tok.value = val;
    return tok;
}

// -----------------------------------------------------------------------
// scan_word  —  identifier or keyword
// -----------------------------------------------------------------------
Token SQLTokenizer::scan_word(Cursor& cur) const {
    Token tok;
    tok.line   = cur.line;
    tok.column = cur.column;

    std::string val;
    while (!cur.at_end() &&
           (std::isalnum(static_cast<unsigned char>(cur.peek())) || cur.peek() == '_')) {
        val += cur.advance();
    }

    std::string upper = to_upper(val);
    if (keywords_.count(upper)) {
        tok.type  = TokenType::KEYWORD;
        tok.value = config_.uppercase_keywords ? upper : val;
    } else {
        tok.type  = TokenType::IDENTIFIER;
        tok.value = val;
    }
    return tok;
}

// -----------------------------------------------------------------------
// scan_line_comment  —  -- until end of line
// -----------------------------------------------------------------------
Token SQLTokenizer::scan_line_comment(Cursor& cur) const {
    Token tok;
    tok.type   = TokenType::LINE_COMMENT;
    tok.line   = cur.line;
    tok.column = cur.column;

    std::string val;
    val += cur.advance(); // -
    val += cur.advance(); // -
    while (!cur.at_end() && cur.peek() != '\n') {
        val += cur.advance();
    }
    tok.value = val;
    return tok;
}

// -----------------------------------------------------------------------
// scan_block_comment  —  /* … */   (supports nesting)
// -----------------------------------------------------------------------
Token SQLTokenizer::scan_block_comment(Cursor& cur) const {
    Token tok;
    tok.type   = TokenType::BLOCK_COMMENT;
    tok.line   = cur.line;
    tok.column = cur.column;

    std::string val;
    val += cur.advance(); // /
    val += cur.advance(); // *

    int depth = 1;
    while (!cur.at_end() && depth > 0) {
        if (cur.peek() == '/' && cur.peek_next() == '*') {
            val += cur.advance();
            val += cur.advance();
            ++depth;
        } else if (cur.peek() == '*' && cur.peek_next() == '/') {
            val += cur.advance();
            val += cur.advance();
            --depth;
        } else {
            val += cur.advance();
        }
    }

    tok.value = val;
    return tok;
}

// -----------------------------------------------------------------------
// scan_placeholder  —  ?  :name  $1
// -----------------------------------------------------------------------
Token SQLTokenizer::scan_placeholder(Cursor& cur) const {
    Token tok;
    tok.type   = TokenType::PLACEHOLDER;
    tok.line   = cur.line;
    tok.column = cur.column;

    std::string val;
    char c = cur.advance();
    val += c;

    if (c == '?') {
        // Optionally followed by digits (?1, ?2 …)
        while (!cur.at_end() && std::isdigit(static_cast<unsigned char>(cur.peek()))) {
            val += cur.advance();
        }
    } else if (c == ':') {
        // Named placeholder :name
        while (!cur.at_end() &&
               (std::isalnum(static_cast<unsigned char>(cur.peek())) || cur.peek() == '_')) {
            val += cur.advance();
        }
    } else if (c == '$') {
        // Positional $1 or named $tag$…$tag$ (PostgreSQL dollar-quoting)
        while (!cur.at_end() && std::isdigit(static_cast<unsigned char>(cur.peek()))) {
            val += cur.advance();
        }
    }

    tok.value = val;
    return tok;
}

} // namespace parser
} // namespace sql
} // namespace ml
