#include "Lexer.h"
#include <cctype>
#include <sstream>

namespace algoc {

std::string Token::toString() const {
    std::ostringstream oss;
    oss << "[" << location.line << ":" << location.column << "] ";
    switch (kind) {
        case tok_eof: oss << "EOF"; break;
        case tok_identifier: oss << "IDENTIFIER(" << text << ")"; break;
        case tok_int_literal: oss << "INT_LITERAL(" << intVal << ")"; break;
        case tok_float_literal: oss << "FLOAT_LITERAL(" << floatVal << ")"; break;
        case tok_bool_literal: oss << "BOOL_LITERAL(" << (boolVal ? "true" : "false") << ")"; break;
        case tok_string_literal: oss << "STRING_LITERAL(\"" << text << "\")"; break;
        case tok_int: oss << "KEYWORD(int)"; break;
        case tok_float: oss << "KEYWORD(float)"; break;
        case tok_bool: oss << "KEYWORD(bool)"; break;
        case tok_void: oss << "KEYWORD(void)"; break;
        case tok_if: oss << "KEYWORD(if)"; break;
        case tok_else: oss << "KEYWORD(else)"; break;
        case tok_while: oss << "KEYWORD(while)"; break;
        case tok_return: oss << "KEYWORD(return)"; break;
        case tok_eq: oss << "OP(==)"; break;
        case tok_ne: oss << "OP(!=)"; break;
        case tok_le: oss << "OP(<=)"; break;
        case tok_ge: oss << "OP(>=)"; break;
        case tok_and: oss << "OP(&&)"; break;
        case tok_or: oss << "OP(||)"; break;
        default:
            if (kind > 0 && kind < 128) {
                oss << "PUNCT('" << static_cast<char>(kind) << "')";
            } else {
                oss << "UNKNOWN(" << kind << ")";
            }
            break;
    }
    return oss.str();
}

Lexer::Lexer(const std::string &source, const std::string &filename)
    : source(source), filename(filename) {}

char Lexer::peekChar() const {
    if (cursor >= source.size()) return '\0';
    return source[cursor];
}

char Lexer::getChar() {
    if (cursor >= source.size()) return '\0';
    char c = source[cursor++];
    if (c == '\n') {
        currentLine++;
        currentColumn = 1;
    } else {
        currentColumn++;
    }
    return c;
}

void Lexer::skipWhitespaceAndComments() {
    while (cursor < source.size()) {
        char c = peekChar();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            getChar();
        } else if (c == '/' && cursor + 1 < source.size()) {
            char next = source[cursor + 1];
            if (next == '/') {
                // Line comment
                getChar(); // '/'
                getChar(); // '/'
                while (cursor < source.size() && peekChar() != '\n') {
                    getChar();
                }
            } else if (next == '*') {
                // Block comment
                getChar(); // '/'
                getChar(); // '*'
                while (cursor < source.size()) {
                    if (peekChar() == '*' && cursor + 1 < source.size() && source[cursor + 1] == '/') {
                        getChar(); // '*'
                        getChar(); // '/'
                        break;
                    }
                    getChar();
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

Token Lexer::readIdentifierOrKeyword(const SourceLocation &loc) {
    std::string ident;
    while (cursor < source.size()) {
        char c = peekChar();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            ident += getChar();
        } else {
            break;
        }
    }

    Token tok;
    tok.location = loc;
    tok.text = ident;

    if (ident == "int") tok.kind = tok_int;
    else if (ident == "float") tok.kind = tok_float;
    else if (ident == "bool") tok.kind = tok_bool;
    else if (ident == "void") tok.kind = tok_void;
    else if (ident == "if") tok.kind = tok_if;
    else if (ident == "else") tok.kind = tok_else;
    else if (ident == "while") tok.kind = tok_while;
    else if (ident == "return") tok.kind = tok_return;
    else if (ident == "true") {
        tok.kind = tok_bool_literal;
        tok.boolVal = true;
    } else if (ident == "false") {
        tok.kind = tok_bool_literal;
        tok.boolVal = false;
    } else {
        tok.kind = tok_identifier;
    }

    return tok;
}

Token Lexer::readNumber(const SourceLocation &loc) {
    std::string numStr;
    bool isFloat = false;

    while (cursor < source.size()) {
        char c = peekChar();
        if (std::isdigit(static_cast<unsigned char>(c))) {
            numStr += getChar();
        } else if (c == '.' && !isFloat && cursor + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor + 1]))) {
            isFloat = true;
            numStr += getChar();
        } else {
            break;
        }
    }

    Token tok;
    tok.location = loc;
    tok.text = numStr;
    if (isFloat) {
        tok.kind = tok_float_literal;
        tok.floatVal = std::stod(numStr);
    } else {
        tok.kind = tok_int_literal;
        tok.intVal = std::stoll(numStr);
    }
    return tok;
}

Token Lexer::readString(const SourceLocation &loc) {
    getChar(); // skip opening '"'
    std::string str;
    while (cursor < source.size() && peekChar() != '"') {
        char c = getChar();
        if (c == '\\' && cursor < source.size()) {
            char esc = getChar();
            if (esc == 'n') str += '\n';
            else if (esc == 't') str += '\t';
            else if (esc == 'r') str += '\r';
            else if (esc == '\\') str += '\\';
            else if (esc == '"') str += '"';
            else str += esc;
        } else {
            str += c;
        }
    }
    if (peekChar() == '"') {
        getChar(); // skip closing '"'
    }

    Token tok;
    tok.kind = tok_string_literal;
    tok.location = loc;
    tok.text = str;
    return tok;
}

Token Lexer::getNextToken() {
    if (hasLookahead) {
        hasLookahead = false;
        return currentToken;
    }

    skipWhitespaceAndComments();

    SourceLocation loc{currentLine, currentColumn};

    if (cursor >= source.size()) {
        Token tok;
        tok.kind = tok_eof;
        tok.location = loc;
        tok.text = "";
        return tok;
    }

    char c = peekChar();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return readIdentifierOrKeyword(loc);
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        return readNumber(loc);
    }

    if (c == '"') {
        return readString(loc);
    }

    // Check multi-character operators
    if (cursor + 1 < source.size()) {
        char next = source[cursor + 1];
        if (c == '=' && next == '=') {
            getChar(); getChar();
            return Token{tok_eq, "==", 0, 0.0, false, loc};
        }
        if (c == '!' && next == '=') {
            getChar(); getChar();
            return Token{tok_ne, "!=", 0, 0.0, false, loc};
        }
        if (c == '<' && next == '=') {
            getChar(); getChar();
            return Token{tok_le, "<=", 0, 0.0, false, loc};
        }
        if (c == '>' && next == '=') {
            getChar(); getChar();
            return Token{tok_ge, ">=", 0, 0.0, false, loc};
        }
        if (c == '&' && next == '&') {
            getChar(); getChar();
            return Token{tok_and, "&&", 0, 0.0, false, loc};
        }
        if (c == '|' && next == '|') {
            getChar(); getChar();
            return Token{tok_or, "||", 0, 0.0, false, loc};
        }
    }

    // Single character token
    char single = getChar();
    return Token{single, std::string(1, single), 0, 0.0, false, loc};
}

const Token &Lexer::peekToken() {
    if (!hasLookahead) {
        currentToken = getNextToken();
        hasLookahead = true;
    }
    return currentToken;
}

} // namespace algoc

