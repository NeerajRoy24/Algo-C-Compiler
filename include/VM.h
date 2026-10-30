#ifndef ALGO_C_VM_H
#define ALGO_C_VM_H

#include "AST.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <iostream>
#include <cstdint>
#include <memory>

namespace algoc {

enum OpCode {
    OP_CONST_INT,
    OP_CONST_FLOAT,
    OP_CONST_BOOL,
    OP_CONST_STR,

    OP_LOAD_LOCAL,
    OP_STORE_LOCAL,

    OP_ARRAY_ALLOC,
    OP_ARRAY_LOAD,
    OP_ARRAY_STORE,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,

    OP_AND,
    OP_OR,
    OP_NOT,
    OP_NEG,

    OP_JUMP,
    OP_JUMP_IF_FALSE,

    OP_CALL,
    OP_RET,

    OP_PRINT_INT,
    OP_PRINT_FLOAT,
    OP_PRINT_BOOL,
    OP_PRINT_STR,
    OP_PRINTLN,
    OP_READ_INT,

    OP_POP,
    OP_HALT
};

struct Instruction {
    OpCode op;
    int64_t intOperand = 0;
    double floatOperand = 0.0;
    std::string strOperand;
};

using Value = std::variant<int64_t, double, bool, std::string, std::shared_ptr<std::vector<int64_t>>>;

struct CompiledFunction {
    std::string name;
    int numParams = 0;
    std::vector<Instruction> code;
    std::unordered_map<std::string, int> localSlots;
};

class VMCompiler : public ASTVisitor {
public:
    VMCompiler();

    std::unordered_map<std::string, CompiledFunction> compile(ModuleAST &module);

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
    std::unordered_map<std::string, CompiledFunction> compiledFunctions;
    CompiledFunction *currentFn = nullptr;
    int nextLocalSlot = 0;

    void emit(OpCode op, int64_t i = 0, double f = 0.0, const std::string &s = "");
    int emitJump(OpCode op);
    void patchJump(int instructionIndex);
};

class VirtualMachine {
public:
    VirtualMachine() = default;

    int run(const std::unordered_map<std::string, CompiledFunction> &functions,
            const std::string &entryPoint = "main");

private:
    std::vector<Value> operandStack;
    std::vector<std::vector<Value>> callFrameLocals;
};

} // namespace algoc

#endif // ALGO_C_VM_H
