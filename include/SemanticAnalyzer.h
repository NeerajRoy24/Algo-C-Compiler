#ifndef ALGO_C_SEMANTIC_ANALYZER_H
#define ALGO_C_SEMANTIC_ANALYZER_H

#include "AST.h"
#include "SymbolTable.h"
#include <string>
#include <vector>

namespace algoc {

class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer();

    bool analyze(ModuleAST &module);

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string> &getErrors() const { return errors; }
    const SymbolTable &getSymbolTable() const { return symTab; }

    // ASTVisitor implementations
    void visit(NumberExpr &node) override;
    void visit(BoolExpr &node) override;
    void visit(StringLiteralExpr &node) override;
    void visit(VariableExpr &node) override;
    void visit(ArrayAccessExpr &node) override;
    void visit(UnaryExpr &node) override;
    void visit(BinaryExpr &node) override;
    void visit(AssignExpr &node) override;
    void visit(CallExpr &node) override;

    void visit(BlockStmt &node) override;
    void visit(VarDeclStmt &node) override;
    void visit(IfStmt &node) override;
    void visit(WhileStmt &node) override;
    void visit(ReturnStmt &node) override;
    void visit(ExprStmt &node) override;

    void visit(PrototypeAST &node) override;
    void visit(FunctionAST &node) override;
    void visit(ModuleAST &node) override;

private:
    SymbolTable symTab;
    std::vector<std::string> errors;
    TypeInfo currentFunctionReturnType{TypeKind::Void};
    std::string currentFunctionName;
    bool currentBlockHasReturn = false;

    void reportSemanticError(const std::string &msg, const SourceLocation &loc);
    bool checkDefiniteReturn(StmtAST *stmt);
    bool isTypeCompatible(const TypeInfo &target, const TypeInfo &source);
    void registerBuiltins();
};

} // namespace algoc

#endif // ALGO_C_SEMANTIC_ANALYZER_H

