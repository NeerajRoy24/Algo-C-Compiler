#include "Parser.h"
#include <iostream>
#include <sstream>

namespace algoc {

Parser::Parser(Lexer &lexer) : lexer(lexer) {
    getNextToken();
}

void Parser::getNextToken() {
    currentToken = lexer.getNextToken();
}

void Parser::reportError(const std::string &message, const SourceLocation &loc) {
    std::ostringstream oss;
    oss << "Syntax Error at [" << loc.line << ":" << loc.column << "]: " << message;
    errors.push_back(oss.str());
}

int Parser::getTokPrecedence() {
    switch (currentToken.kind) {
        case '*':
        case '/':
        case '%':
            return 50;
        case '+':
        case '-':
            return 40;
        case '<':
        case '>':
        case tok_le:
        case tok_ge:
            return 30;
        case tok_eq:
        case tok_ne:
            return 20;
        case tok_and:
            return 10;
        case tok_or:
            return 5;
        default:
            return -1;
    }
}

TypeInfo Parser::parseType() {
    TypeInfo type;
    if (currentToken.kind == tok_int) {
        type.kind = TypeKind::Int;
    } else if (currentToken.kind == tok_float) {
        type.kind = TypeKind::Float;
    } else if (currentToken.kind == tok_bool) {
        type.kind = TypeKind::Bool;
    } else if (currentToken.kind == tok_void) {
        type.kind = TypeKind::Void;
    } else {
        reportError("Expected type name ('int', 'float', 'bool', 'void')", currentToken.location);
        type.kind = TypeKind::Error;
        return type;
    }
    getNextToken();
    return type;
}

std::unique_ptr<ModuleAST> Parser::parseModule() {
    auto module = std::make_unique<ModuleAST>("algo_c_module");
    while (currentToken.kind != tok_eof) {
        auto fn = parseFunction();
        if (fn) {
            module->addFunction(std::move(fn));
        } else {
            // Error recovery: advance to next function or eof
            getNextToken();
        }
    }
    return module;
}

std::unique_ptr<FunctionAST> Parser::parseFunction() {
    SourceLocation loc = currentToken.location;
    auto proto = parsePrototype();
    if (!proto) return nullptr;

    if (currentToken.kind != '{') {
        reportError("Expected '{' for function body", currentToken.location);
        return nullptr;
    }

    auto body = parseBlock();
    if (!body) return nullptr;

    return std::make_unique<FunctionAST>(std::move(proto), std::move(body), loc);
}

std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
    SourceLocation loc = currentToken.location;
    TypeInfo retType = parseType();
    if (retType.kind == TypeKind::Error) return nullptr;

    if (currentToken.kind != tok_identifier) {
        reportError("Expected function name", currentToken.location);
        return nullptr;
    }
    std::string fnName = currentToken.text;
    getNextToken(); // eat identifier

    if (currentToken.kind != '(') {
        reportError("Expected '(' after function name", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat '('

    std::vector<ParamInfo> params;
    if (currentToken.kind != ')') {
        while (true) {
            SourceLocation pLoc = currentToken.location;
            TypeInfo pType = parseType();
            if (pType.kind == TypeKind::Error) return nullptr;

            if (currentToken.kind != tok_identifier) {
                reportError("Expected parameter name", currentToken.location);
                return nullptr;
            }
            std::string pName = currentToken.text;
            getNextToken(); // eat identifier

            // Check if array parameter: int arr[]
            if (currentToken.kind == '[') {
                getNextToken();
                if (currentToken.kind != ']') {
                    reportError("Expected ']' in array parameter declaration", currentToken.location);
                    return nullptr;
                }
                getNextToken();
                TypeInfo arrType;
                arrType.kind = TypeKind::Array;
                arrType.elementType = pType.kind;
                arrType.arraySize = 0; // unsized slice/pointer
                pType = arrType;
            }

            params.push_back({pType, pName, pLoc});

            if (currentToken.kind == ')') break;
            if (currentToken.kind == ',') {
                getNextToken();
            } else {
                reportError("Expected ',' or ')' in parameter list", currentToken.location);
                return nullptr;
            }
        }
    }
    getNextToken(); // eat ')'
    return std::make_unique<PrototypeAST>(retType, fnName, std::move(params), loc);
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    SourceLocation loc = currentToken.location;
    if (currentToken.kind != '{') {
        reportError("Expected '{' to begin block", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat '{'

    std::vector<std::unique_ptr<StmtAST>> stmts;
    while (currentToken.kind != '}' && currentToken.kind != tok_eof) {
        auto stmt = parseStatement();
        if (stmt) {
            stmts.push_back(std::move(stmt));
        }
    }

    if (currentToken.kind != '}') {
        reportError("Expected '}' at end of block", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat '}'
    return std::make_unique<BlockStmt>(std::move(stmts), loc);
}

std::unique_ptr<StmtAST> Parser::parseStatement() {
    switch (currentToken.kind) {
        case '{':
            return parseBlock();
        case tok_int:
        case tok_float:
        case tok_bool:
        case tok_void:
            return parseVarDecl();
        case tok_if:
            return parseIfStatement();
        case tok_while:
            return parseWhileStatement();
        case tok_return:
            return parseReturnStatement();
        default:
            return parseExprStatement();
    }
}

std::unique_ptr<StmtAST> Parser::parseVarDecl() {
    SourceLocation loc = currentToken.location;
    TypeInfo baseType = parseType();

    if (currentToken.kind != tok_identifier) {
        reportError("Expected variable name after type", currentToken.location);
        return nullptr;
    }
    std::string name = currentToken.text;
    getNextToken(); // eat identifier

    // Check for array declaration: int arr[10];
    if (currentToken.kind == '[') {
        getNextToken(); // eat '['
        if (currentToken.kind != tok_int_literal) {
            reportError("Expected integer size for array declaration", currentToken.location);
            return nullptr;
        }
        int size = static_cast<int>(currentToken.intVal);
        getNextToken(); // eat size
        if (currentToken.kind != ']') {
            reportError("Expected ']' in array declaration", currentToken.location);
            return nullptr;
        }
        getNextToken(); // eat ']'

        if (currentToken.kind != ';') {
            reportError("Expected ';' after array declaration", currentToken.location);
            return nullptr;
        }
        getNextToken(); // eat ';'

        TypeInfo arrType;
        arrType.kind = TypeKind::Array;
        arrType.elementType = baseType.kind;
        arrType.arraySize = size;
        return std::make_unique<VarDeclStmt>(arrType, name, nullptr, loc);
    }

    // Scalar variable
    std::unique_ptr<ExprAST> init = nullptr;
    if (currentToken.kind == '=') {
        getNextToken(); // eat '='
        init = parseExpression();
        if (!init) return nullptr;
    }

    if (currentToken.kind != ';') {
        reportError("Expected ';' after variable declaration", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat ';'

    return std::make_unique<VarDeclStmt>(baseType, name, std::move(init), loc);
}

std::unique_ptr<StmtAST> Parser::parseIfStatement() {
    SourceLocation loc = currentToken.location;
    getNextToken(); // eat 'if'

    if (currentToken.kind != '(') {
        reportError("Expected '(' after 'if'", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat '('

    auto cond = parseExpression();
    if (!cond) return nullptr;

    if (currentToken.kind != ')') {
        reportError("Expected ')' after if condition", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat ')'

    auto thenStmt = parseStatement();
    if (!thenStmt) return nullptr;

    std::unique_ptr<StmtAST> elseStmt = nullptr;
    if (currentToken.kind == tok_else) {
        getNextToken(); // eat 'else'
        elseStmt = parseStatement();
        if (!elseStmt) return nullptr;
    }

    return std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), std::move(elseStmt), loc);
}

std::unique_ptr<StmtAST> Parser::parseWhileStatement() {
    SourceLocation loc = currentToken.location;
    getNextToken(); // eat 'while'

    if (currentToken.kind != '(') {
        reportError("Expected '(' after 'while'", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat '('

    auto cond = parseExpression();
    if (!cond) return nullptr;

    if (currentToken.kind != ')') {
        reportError("Expected ')' after while condition", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat ')'

    auto body = parseStatement();
    if (!body) return nullptr;

    return std::make_unique<WhileStmt>(std::move(cond), std::move(body), loc);
}

std::unique_ptr<StmtAST> Parser::parseReturnStatement() {
    SourceLocation loc = currentToken.location;
    getNextToken(); // eat 'return'

    std::unique_ptr<ExprAST> val = nullptr;
    if (currentToken.kind != ';') {
        val = parseExpression();
        if (!val) return nullptr;
    }

    if (currentToken.kind != ';') {
        reportError("Expected ';' after return statement", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat ';'

    return std::make_unique<ReturnStmt>(std::move(val), loc);
}

std::unique_ptr<StmtAST> Parser::parseExprStatement() {
    SourceLocation loc = currentToken.location;
    auto expr = parseExpression();
    if (!expr) return nullptr;

    if (currentToken.kind != ';') {
        reportError("Expected ';' after expression", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat ';'

    return std::make_unique<ExprStmt>(std::move(expr), loc);
}

std::unique_ptr<ExprAST> Parser::parseExpression() {
    auto lhs = parseUnaryExpr();
    if (!lhs) return nullptr;
    return parseBinOpRHS(0, std::move(lhs));
}

std::unique_ptr<ExprAST> Parser::parseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST> lhs) {
    while (true) {
        int tokPrec = getTokPrecedence();
        if (tokPrec < exprPrec) {
            return lhs;
        }

        int binOp = currentToken.kind;
        SourceLocation opLoc = currentToken.location;
        getNextToken(); // eat binary operator

        auto rhs = parseUnaryExpr();
        if (!rhs) return nullptr;

        int nextPrec = getTokPrecedence();
        if (tokPrec < nextPrec) {
            rhs = parseBinOpRHS(tokPrec + 1, std::move(rhs));
            if (!rhs) return nullptr;
        }

        lhs = std::make_unique<BinaryExpr>(binOp, std::move(lhs), std::move(rhs), opLoc);
    }
}

std::unique_ptr<ExprAST> Parser::parseUnaryExpr() {
    SourceLocation loc = currentToken.location;
    if (currentToken.kind == '-' || currentToken.kind == '!') {
        int op = currentToken.kind;
        getNextToken();
        auto operand = parseUnaryExpr();
        if (!operand) return nullptr;
        return std::make_unique<UnaryExpr>(op, std::move(operand), loc);
    }
    return parsePrimary();
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
    SourceLocation loc = currentToken.location;
    switch (currentToken.kind) {
        case tok_identifier:
            return parseIdentifierExpr();
        case tok_int_literal: {
            int64_t val = currentToken.intVal;
            getNextToken();
            return std::make_unique<NumberExpr>(val, loc);
        }
        case tok_float_literal: {
            double val = currentToken.floatVal;
            getNextToken();
            return std::make_unique<NumberExpr>(val, loc);
        }
        case tok_bool_literal: {
            bool val = currentToken.boolVal;
            getNextToken();
            return std::make_unique<BoolExpr>(val, loc);
        }
        case tok_string_literal: {
            std::string val = currentToken.text;
            getNextToken();
            return std::make_unique<StringLiteralExpr>(val, loc);
        }
        case '(':
            return parseParenExpr();
        default:
            reportError("Unexpected token in expression: " + currentToken.text, loc);
            return nullptr;
    }
}

std::unique_ptr<ExprAST> Parser::parseIdentifierExpr() {
    SourceLocation loc = currentToken.location;
    std::string idName = currentToken.text;
    getNextToken(); // eat identifier

    // 1. Function Call: foo(...)
    if (currentToken.kind == '(') {
        getNextToken(); // eat '('
        std::vector<std::unique_ptr<ExprAST>> args;
        if (currentToken.kind != ')') {
            while (true) {
                auto arg = parseExpression();
                if (!arg) return nullptr;
                args.push_back(std::move(arg));

                if (currentToken.kind == ')') break;
                if (currentToken.kind == ',') {
                    getNextToken();
                } else {
                    reportError("Expected ')' or ',' in argument list", currentToken.location);
                    return nullptr;
                }
            }
        }
        getNextToken(); // eat ')'
        return std::make_unique<CallExpr>(idName, std::move(args), loc);
    }

    // 2. Array Access or Array Assignment: arr[index] or arr[index] = expr
    if (currentToken.kind == '[') {
        getNextToken(); // eat '['
        auto index = parseExpression();
        if (!index) return nullptr;

        if (currentToken.kind != ']') {
            reportError("Expected ']' in array subscript", currentToken.location);
            return nullptr;
        }
        getNextToken(); // eat ']'

        if (currentToken.kind == '=') {
            // Array assignment: arr[idx] = expr
            getNextToken(); // eat '='
            auto rhs = parseExpression();
            if (!rhs) return nullptr;
            return std::make_unique<AssignExpr>(idName, std::move(index), std::move(rhs), loc);
        }

        return std::make_unique<ArrayAccessExpr>(idName, std::move(index), loc);
    }

    // 3. Variable Assignment: x = expr
    if (currentToken.kind == '=') {
        getNextToken(); // eat '='
        auto rhs = parseExpression();
        if (!rhs) return nullptr;
        return std::make_unique<AssignExpr>(idName, std::move(rhs), loc);
    }

    // 4. Simple Variable: x
    return std::make_unique<VariableExpr>(idName, loc);
}

std::unique_ptr<ExprAST> Parser::parseParenExpr() {
    getNextToken(); // eat '('
    auto expr = parseExpression();
    if (!expr) return nullptr;

    if (currentToken.kind != ')') {
        reportError("Expected ')' to close expression", currentToken.location);
        return nullptr;
    }
    getNextToken(); // eat ')'
    return expr;
}

} // namespace algoc

