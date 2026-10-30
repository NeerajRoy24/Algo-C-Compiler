#ifndef ALGO_C_CODEGEN_H
#define ALGO_C_CODEGEN_H

#include "AST.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

namespace algoc {

struct LLVMValue {
    std::string name;
    TypeInfo type;
    bool isPointer = false;
};

struct BasicBlock {
    std::string label;
    std::vector<std::string> instructions;
    bool hasTerminator = false;
};

struct FunctionIR {
    std::string name;
    TypeInfo returnType;
    std::vector<ParamInfo> params;
    std::vector<BasicBlock> blocks;
    int currentBlockIndex = 0;
};

class CodeGen : public ASTVisitor {
public:
    CodeGen();

    std::string generateIR(ModuleAST &module);

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
    std::ostringstream moduleStream;
    std::vector<FunctionIR> functions;
    int currentFunctionIndex = -1;

    // Scoped variable mappings: varName -> LLVMValue (alloca slot)
    std::vector<std::unordered_map<std::string, LLVMValue>> namedValuesScope;

    int tempRegisterCount = 0;
    int labelCount = 0;
    int stringLiteralCount = 0;
    std::vector<std::pair<std::string, std::string>> stringConstants; // global string constants

    LLVMValue lastValue;

    std::string newRegister();
    std::string newLabel(const std::string &prefix = "bb");
    void enterScope();
    void exitScope();
    void setVariable(const std::string &name, const LLVMValue &val);
    LLVMValue getVariable(const std::string &name);

    void emitInstruction(const std::string &inst);
    void createBlock(const std::string &label);
    void switchToBlock(const std::string &label);
    BasicBlock *getCurrentBlock();

    std::string llvmType(const TypeInfo &type);
};

} // namespace algoc

#endif // ALGO_C_CODEGEN_H

