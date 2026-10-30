#ifndef ALGO_C_PARSER_H
#define ALGO_C_PARSER_H

#include "Lexer.h"
#include "AST.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace algoc {

class Parser {
public:
    explicit Parser(Lexer &lexer);

    std::unique_ptr<ModuleAST> parseModule();

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string> &getErrors() const { return errors; }

private:
    Lexer &lexer;
    Token currentToken;
    std::vector<std::string> errors;

    void getNextToken();
    void reportError(const std::string &message, const SourceLocation &loc);

    int getTokPrecedence();

    // Recursive-descent grammar methods
    std::unique_ptr<FunctionAST> parseFunction();
    std::unique_ptr<PrototypeAST> parsePrototype();
    std::unique_ptr<StmtAST> parseStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<StmtAST> parseVarDecl();
    std::unique_ptr<StmtAST> parseIfStatement();
    std::unique_ptr<StmtAST> parseWhileStatement();
    std::unique_ptr<StmtAST> parseReturnStatement();
    std::unique_ptr<StmtAST> parseExprStatement();

    std::unique_ptr<ExprAST> parseExpression();
    std::unique_ptr<ExprAST> parsePrimary();
    std::unique_ptr<ExprAST> parseUnaryExpr();
    std::unique_ptr<ExprAST> parseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST> lhs);
    std::unique_ptr<ExprAST> parseIdentifierExpr();
    std::unique_ptr<ExprAST> parseParenExpr();

    TypeInfo parseType();
};

} // namespace algoc

#endif // ALGO_C_PARSER_H

