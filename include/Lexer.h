#ifndef ALGO_C_LEXER_H
#define ALGO_C_LEXER_H

#include <string>
#include <vector>
#include <iostream>
#include <cstdint>

namespace algoc {

enum TokenKind {
    tok_eof = -1,
    tok_identifier = -2,
    tok_int_literal = -3,
    tok_float_literal = -4,
    tok_bool_literal = -5,
    tok_string_literal = -6,

    // Types
    tok_int = -10,
    tok_float = -11,
    tok_bool = -12,
    tok_void = -13,

    // Control flow
    tok_if = -20,
    tok_else = -21,
    tok_while = -22,
    tok_return = -23,

    // Multi-char operators
    tok_eq = -30,       // ==
    tok_ne = -31,       // !=
    tok_le = -32,       // <=
    tok_ge = -33,       // >=
    tok_and = -34,      // &&
    tok_or = -35,       // ||

    // Single-char operators and punctuation use their ASCII values:
    // '+', '-', '*', '/', '%', '=', '<', '>', '!', '(', ')', '{', '}', '[', ']', ';', ','
};

struct SourceLocation {
    int line = 1;
    int column = 1;
};

struct Token {
    int kind;
    std::string text;
    int64_t intVal = 0;
    double floatVal = 0.0;
    bool boolVal = false;
    SourceLocation location;

    std::string toString() const;
};

class Lexer {
public:
    explicit Lexer(const std::string &source, const std::string &filename = "<input>");

    Token getNextToken();
    const Token &peekToken();
    const std::string &getFilename() const { return filename; }

private:
    std::string source;
    std::string filename;
    size_t cursor = 0;
    int currentLine = 1;
    int currentColumn = 1;
    Token currentToken;
    bool hasLookahead = false;

    char peekChar() const;
    char getChar();
    void skipWhitespaceAndComments();
    Token readIdentifierOrKeyword(const SourceLocation &loc);
    Token readNumber(const SourceLocation &loc);
    Token readString(const SourceLocation &loc);
};

} // namespace algoc

#endif // ALGO_C_LEXER_H
