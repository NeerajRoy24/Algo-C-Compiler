#ifndef ALGO_C_AST_H
#define ALGO_C_AST_H

#include "Lexer.h"
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <cstdint>

namespace algoc {

enum class TypeKind {
    Void,
    Int,
    Float,
    Bool,
    String,
    Array,
    Function,
    Error
};

struct TypeInfo {
    TypeKind kind = TypeKind::Void;
    TypeKind elementType = TypeKind::Void; // for Array
    int arraySize = 0;                     // for fixed size arrays

    bool isNumeric() const { return kind == TypeKind::Int || kind == TypeKind::Float; }
    bool isInt() const { return kind == TypeKind::Int; }
    bool isFloat() const { return kind == TypeKind::Float; }
    bool isBool() const { return kind == TypeKind::Bool; }
    bool isVoid() const { return kind == TypeKind::Void; }
    bool isArray() const { return kind == TypeKind::Array; }
    bool isString() const { return kind == TypeKind::String; }

    bool operator==(const TypeInfo &other) const {
        if (kind != other.kind) return false;
        if (kind == TypeKind::Array) {
            return elementType == other.elementType && arraySize == other.arraySize;
        }
        return true;
    }
    bool operator!=(const TypeInfo &other) const { return !(*this == other); }

    std::string toString() const;
};

enum NodeKind {
    Node_ASTNode,

    // Expression ranges
    Node_ExprFirst,
    Node_NumberExpr,
    Node_BoolExpr,
    Node_StringLiteralExpr,
    Node_VariableExpr,
    Node_ArrayAccessExpr,
    Node_UnaryExpr,
    Node_BinaryExpr,
    Node_AssignExpr,
    Node_CallExpr,
    Node_ExprLast,

    // Statement ranges
    Node_StmtFirst,
    Node_BlockStmt,
    Node_VarDeclStmt,
    Node_IfStmt,
    Node_WhileStmt,
    Node_ReturnStmt,
    Node_ExprStmt,
    Node_StmtLast,

    // Top level
    Node_PrototypeAST,
    Node_FunctionAST,
    Node_ModuleAST
};

class ASTVisitor;

class ASTNode {
public:
    explicit ASTNode(NodeKind kind, SourceLocation loc = {}) : kind(kind), location(loc) {}
    virtual ~ASTNode() = default;

    NodeKind getKind() const { return kind; }
    SourceLocation getLocation() const { return location; }

    virtual void accept(ASTVisitor &visitor) = 0;
    virtual void dump(int indent = 0) const = 0;

    static bool classof(const ASTNode *N) { (void)N; return true; }

private:
    NodeKind kind;
    SourceLocation location;
};

// LLVM-style RTTI helpers
template <typename To, typename From>
bool isa(const From *Val) {
    return Val && To::classof(Val);
}

template <typename To, typename From>
To *cast(From *Val) {
    assert(isa<To>(Val) && "Invalid cast!");
    return static_cast<To *>(Val);
}

template <typename To, typename From>
const To *cast(const From *Val) {
    assert(isa<To>(Val) && "Invalid cast!");
    return static_cast<const To *>(Val);
}

template <typename To, typename From>
To *dyn_cast(From *Val) {
    return isa<To>(Val) ? static_cast<To *>(Val) : nullptr;
}

template <typename To, typename From>
const To *dyn_cast(const From *Val) {
    return isa<To>(Val) ? static_cast<const To *>(Val) : nullptr;
}

// Base Expression
class ExprAST : public ASTNode {
public:
    ExprAST(NodeKind kind, SourceLocation loc) : ASTNode(kind, loc) {}

    const TypeInfo &getType() const { return inferredType; }
    void setType(const TypeInfo &t) { inferredType = t; }

    static bool classof(const ASTNode *N) {
        return N->getKind() >= Node_ExprFirst && N->getKind() <= Node_ExprLast;
    }

protected:
    TypeInfo inferredType{TypeKind::Void};
};

class NumberExpr : public ExprAST {
public:
    NumberExpr(int64_t val, SourceLocation loc)
        : ExprAST(Node_NumberExpr, loc), isFloat(false), intVal(val), floatVal(0.0) {
        setType({TypeKind::Int});
    }
    NumberExpr(double val, SourceLocation loc)
        : ExprAST(Node_NumberExpr, loc), isFloat(true), intVal(0), floatVal(val) {
        setType({TypeKind::Float});
    }

    bool getIsFloat() const { return isFloat; }
    int64_t getIntVal() const { return intVal; }
    double getFloatVal() const { return floatVal; }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_NumberExpr; }

private:
    bool isFloat;
    int64_t intVal;
    double floatVal;
};

class BoolExpr : public ExprAST {
public:
    BoolExpr(bool val, SourceLocation loc)
        : ExprAST(Node_BoolExpr, loc), value(val) {
        setType({TypeKind::Bool});
    }

    bool getValue() const { return value; }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_BoolExpr; }

private:
    bool value;
};

class StringLiteralExpr : public ExprAST {
public:
    StringLiteralExpr(const std::string &val, SourceLocation loc)
        : ExprAST(Node_StringLiteralExpr, loc), value(val) {
        setType({TypeKind::String});
    }

    const std::string &getValue() const { return value; }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_StringLiteralExpr; }

private:
    std::string value;
};

class VariableExpr : public ExprAST {
public:
    VariableExpr(const std::string &name, SourceLocation loc)
        : ExprAST(Node_VariableExpr, loc), name(name) {}

    const std::string &getName() const { return name; }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_VariableExpr; }

private:
    std::string name;
};

class ArrayAccessExpr : public ExprAST {
public:
    ArrayAccessExpr(const std::string &name, std::unique_ptr<ExprAST> index, SourceLocation loc)
        : ExprAST(Node_ArrayAccessExpr, loc), name(name), index(std::move(index)) {}

    const std::string &getName() const { return name; }
    ExprAST *getIndex() const { return index.get(); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_ArrayAccessExpr; }

private:
    std::string name;
    std::unique_ptr<ExprAST> index;
};

class UnaryExpr : public ExprAST {
public:
    UnaryExpr(int op, std::unique_ptr<ExprAST> operand, SourceLocation loc)
        : ExprAST(Node_UnaryExpr, loc), op(op), operand(std::move(operand)) {}

    int getOp() const { return op; }
    ExprAST *getOperand() const { return operand.get(); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_UnaryExpr; }

private:
    int op; // '-', '!'
    std::unique_ptr<ExprAST> operand;
};

class BinaryExpr : public ExprAST {
public:
    BinaryExpr(int op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs, SourceLocation loc)
        : ExprAST(Node_BinaryExpr, loc), op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    int getOp() const { return op; }
    ExprAST *getLHS() const { return lhs.get(); }
    ExprAST *getRHS() const { return rhs.get(); }

    std::unique_ptr<ExprAST> takeLHS() { return std::move(lhs); }
    std::unique_ptr<ExprAST> takeRHS() { return std::move(rhs); }

    void setLHS(std::unique_ptr<ExprAST> newLhs) { lhs = std::move(newLhs); }
    void setRHS(std::unique_ptr<ExprAST> newRhs) { rhs = std::move(newRhs); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_BinaryExpr; }

private:
    int op;
    std::unique_ptr<ExprAST> lhs;
    std::unique_ptr<ExprAST> rhs;
};

class AssignExpr : public ExprAST {
public:
    AssignExpr(const std::string &varName, std::unique_ptr<ExprAST> val, SourceLocation loc)
        : ExprAST(Node_AssignExpr, loc), varName(varName), value(std::move(val)), isArrayAssign(false) {}

    AssignExpr(const std::string &varName, std::unique_ptr<ExprAST> idx, std::unique_ptr<ExprAST> val, SourceLocation loc)
        : ExprAST(Node_AssignExpr, loc), varName(varName), arrayIndex(std::move(idx)), value(std::move(val)), isArrayAssign(true) {}

    const std::string &getVarName() const { return varName; }
    ExprAST *getValue() const { return value.get(); }
    bool getIsArrayAssign() const { return isArrayAssign; }
    ExprAST *getArrayIndex() const { return arrayIndex.get(); }

    void setValue(std::unique_ptr<ExprAST> newVal) { value = std::move(newVal); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_AssignExpr; }

private:
    std::string varName;
    std::unique_ptr<ExprAST> arrayIndex;
    std::unique_ptr<ExprAST> value;
    bool isArrayAssign;
};

class CallExpr : public ExprAST {
public:
    CallExpr(const std::string &callee, std::vector<std::unique_ptr<ExprAST>> args, SourceLocation loc)
        : ExprAST(Node_CallExpr, loc), callee(callee), args(std::move(args)) {}

    const std::string &getCallee() const { return callee; }
    const std::vector<std::unique_ptr<ExprAST>> &getArgs() const { return args; }
    std::vector<std::unique_ptr<ExprAST>> &getArgs() { return args; }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_CallExpr; }

private:
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;
};

// Base Statement
class StmtAST : public ASTNode {
public:
    StmtAST(NodeKind kind, SourceLocation loc) : ASTNode(kind, loc) {}

    static bool classof(const ASTNode *N) {
        return N->getKind() >= Node_StmtFirst && N->getKind() <= Node_StmtLast;
    }
};

class BlockStmt : public StmtAST {
public:
    BlockStmt(std::vector<std::unique_ptr<StmtAST>> stmts, SourceLocation loc)
        : StmtAST(Node_BlockStmt, loc), statements(std::move(stmts)) {}

    const std::vector<std::unique_ptr<StmtAST>> &getStatements() const { return statements; }
    std::vector<std::unique_ptr<StmtAST>> &getStatements() { return statements; }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_BlockStmt; }

private:
    std::vector<std::unique_ptr<StmtAST>> statements;
};

class VarDeclStmt : public StmtAST {
public:
    // Scalar variable declaration: int x = 5; or int x;
    VarDeclStmt(TypeInfo type, const std::string &name, std::unique_ptr<ExprAST> init, SourceLocation loc)
        : StmtAST(Node_VarDeclStmt, loc), type(type), name(name), init(std::move(init)) {}

    const TypeInfo &getType() const { return type; }
    const std::string &getName() const { return name; }
    ExprAST *getInit() const { return init.get(); }
    std::unique_ptr<ExprAST> takeInit() { return std::move(init); }
    void setInit(std::unique_ptr<ExprAST> newInit) { init = std::move(newInit); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_VarDeclStmt; }

private:
    TypeInfo type;
    std::string name;
    std::unique_ptr<ExprAST> init;
};

class IfStmt : public StmtAST {
public:
    IfStmt(std::unique_ptr<ExprAST> cond, std::unique_ptr<StmtAST> thenStmt,
           std::unique_ptr<StmtAST> elseStmt, SourceLocation loc)
        : StmtAST(Node_IfStmt, loc), cond(std::move(cond)),
          thenStmt(std::move(thenStmt)), elseStmt(std::move(elseStmt)) {}

    ExprAST *getCond() const { return cond.get(); }
    StmtAST *getThenStmt() const { return thenStmt.get(); }
    StmtAST *getElseStmt() const { return elseStmt.get(); }

    std::unique_ptr<ExprAST> takeCond() { return std::move(cond); }
    std::unique_ptr<StmtAST> takeThenStmt() { return std::move(thenStmt); }
    std::unique_ptr<StmtAST> takeElseStmt() { return std::move(elseStmt); }

    void setCond(std::unique_ptr<ExprAST> newCond) { cond = std::move(newCond); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_IfStmt; }

private:
    std::unique_ptr<ExprAST> cond;
    std::unique_ptr<StmtAST> thenStmt;
    std::unique_ptr<StmtAST> elseStmt;
};

class WhileStmt : public StmtAST {
public:
    WhileStmt(std::unique_ptr<ExprAST> cond, std::unique_ptr<StmtAST> body, SourceLocation loc)
        : StmtAST(Node_WhileStmt, loc), cond(std::move(cond)), body(std::move(body)) {}

    ExprAST *getCond() const { return cond.get(); }
    StmtAST *getBody() const { return body.get(); }

    void setCond(std::unique_ptr<ExprAST> newCond) { cond = std::move(newCond); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_WhileStmt; }

private:
    std::unique_ptr<ExprAST> cond;
    std::unique_ptr<StmtAST> body;
};

class ReturnStmt : public StmtAST {
public:
    ReturnStmt(std::unique_ptr<ExprAST> val, SourceLocation loc)
        : StmtAST(Node_ReturnStmt, loc), value(std::move(val)) {}

    ExprAST *getValue() const { return value.get(); }
    void setValue(std::unique_ptr<ExprAST> newVal) { value = std::move(newVal); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_ReturnStmt; }

private:
    std::unique_ptr<ExprAST> value;
};

class ExprStmt : public StmtAST {
public:
    ExprStmt(std::unique_ptr<ExprAST> expr, SourceLocation loc)
        : StmtAST(Node_ExprStmt, loc), expr(std::move(expr)) {}

    ExprAST *getExpr() const { return expr.get(); }
    void setExpr(std::unique_ptr<ExprAST> newExpr) { expr = std::move(newExpr); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_ExprStmt; }

private:
    std::unique_ptr<ExprAST> expr;
};

// Parameter for function prototype
struct ParamInfo {
    TypeInfo type;
    std::string name;
    SourceLocation location;
};

class PrototypeAST : public ASTNode {
public:
    PrototypeAST(TypeInfo returnType, const std::string &name, std::vector<ParamInfo> params, SourceLocation loc)
        : ASTNode(Node_PrototypeAST, loc), returnType(returnType), name(name), params(std::move(params)) {}

    const TypeInfo &getReturnType() const { return returnType; }
    const std::string &getName() const { return name; }
    const std::vector<ParamInfo> &getParams() const { return params; }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_PrototypeAST; }

private:
    TypeInfo returnType;
    std::string name;
    std::vector<ParamInfo> params;
};

class FunctionAST : public ASTNode {
public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto, std::unique_ptr<BlockStmt> body, SourceLocation loc)
        : ASTNode(Node_FunctionAST, loc), proto(std::move(proto)), body(std::move(body)) {}

    PrototypeAST *getProto() const { return proto.get(); }
    BlockStmt *getBody() const { return body.get(); }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_FunctionAST; }

private:
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<BlockStmt> body;
};

class ModuleAST : public ASTNode {
public:
    ModuleAST(const std::string &name = "algo_module")
        : ASTNode(Node_ModuleAST), name(name) {}

    const std::string &getName() const { return name; }
    const std::vector<std::unique_ptr<FunctionAST>> &getFunctions() const { return functions; }
    std::vector<std::unique_ptr<FunctionAST>> &getFunctions() { return functions; }

    void addFunction(std::unique_ptr<FunctionAST> fn) {
        functions.push_back(std::move(fn));
    }

    void accept(ASTVisitor &visitor) override;
    void dump(int indent = 0) const override;

    static bool classof(const ASTNode *N) { return N->getKind() == Node_ModuleAST; }

private:
    std::string name;
    std::vector<std::unique_ptr<FunctionAST>> functions;
};

// Visitor Interface
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(NumberExpr &node) = 0;
    virtual void visit(BoolExpr &node) = 0;
    virtual void visit(StringLiteralExpr &node) = 0;
    virtual void visit(VariableExpr &node) = 0;
    virtual void visit(ArrayAccessExpr &node) = 0;
    virtual void visit(UnaryExpr &node) = 0;
    virtual void visit(BinaryExpr &node) = 0;
    virtual void visit(AssignExpr &node) = 0;
    virtual void visit(CallExpr &node) = 0;

    virtual void visit(BlockStmt &node) = 0;
    virtual void visit(VarDeclStmt &node) = 0;
    virtual void visit(IfStmt &node) = 0;
    virtual void visit(WhileStmt &node) = 0;
    virtual void visit(ReturnStmt &node) = 0;
    virtual void visit(ExprStmt &node) = 0;

    virtual void visit(PrototypeAST &node) = 0;
    virtual void visit(FunctionAST &node) = 0;
    virtual void visit(ModuleAST &node) = 0;
};

} // namespace algoc

#endif // ALGO_C_AST_H
