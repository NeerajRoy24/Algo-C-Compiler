#include "AST.h"
#include <iostream>

namespace algoc {

std::string TypeInfo::toString() const {
    switch (kind) {
        case TypeKind::Void: return "void";
        case TypeKind::Int: return "int";
        case TypeKind::Float: return "float";
        case TypeKind::Bool: return "bool";
        case TypeKind::String: return "string";
        case TypeKind::Array: {
            std::string elemStr = (elementType == TypeKind::Int ? "int" :
                                  elementType == TypeKind::Float ? "float" :
                                  elementType == TypeKind::Bool ? "bool" : "unknown");
            return elemStr + "[" + std::to_string(arraySize) + "]";
        }
        case TypeKind::Function: return "function";
        case TypeKind::Error: return "error_type";
    }
    return "unknown";
}

static void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) std::cout << "  ";
}

void NumberExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void NumberExpr::dump(int indent) const {
    printIndent(indent);
    if (isFloat) {
        std::cout << "NumberExpr(float: " << floatVal << ") [" << inferredType.toString() << "]\n";
    } else {
        std::cout << "NumberExpr(int: " << intVal << ") [" << inferredType.toString() << "]\n";
    }
}

void BoolExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void BoolExpr::dump(int indent) const {
    printIndent(indent);
    std::cout << "BoolExpr(" << (value ? "true" : "false") << ") [" << inferredType.toString() << "]\n";
}

void StringLiteralExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void StringLiteralExpr::dump(int indent) const {
    printIndent(indent);
    std::cout << "StringLiteralExpr(\"" << value << "\")\n";
}

void VariableExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void VariableExpr::dump(int indent) const {
    printIndent(indent);
    std::cout << "VariableExpr(" << name << ") [" << inferredType.toString() << "]\n";
}

void ArrayAccessExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ArrayAccessExpr::dump(int indent) const {
    printIndent(indent);
    std::cout << "ArrayAccessExpr(" << name << ") [" << inferredType.toString() << "]\n";
    printIndent(indent + 1);
    std::cout << "Index:\n";
    if (index) index->dump(indent + 2);
}

void UnaryExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void UnaryExpr::dump(int indent) const {
    printIndent(indent);
    std::cout << "UnaryExpr(op: " << (char)op << ") [" << inferredType.toString() << "]\n";
    if (operand) operand->dump(indent + 1);
}

void BinaryExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void BinaryExpr::dump(int indent) const {
    printIndent(indent);
    std::string opStr;
    switch (op) {
        case tok_eq: opStr = "=="; break;
        case tok_ne: opStr = "!="; break;
        case tok_le: opStr = "<="; break;
        case tok_ge: opStr = ">="; break;
        case tok_and: opStr = "&&"; break;
        case tok_or: opStr = "||"; break;
        default: opStr = std::string(1, (char)op); break;
    }
    std::cout << "BinaryExpr(" << opStr << ") [" << inferredType.toString() << "]\n";
    if (lhs) lhs->dump(indent + 1);
    if (rhs) rhs->dump(indent + 1);
}

void AssignExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void AssignExpr::dump(int indent) const {
    printIndent(indent);
    if (isArrayAssign) {
        std::cout << "AssignExpr(Array " << varName << "[] = ...)\n";
        printIndent(indent + 1);
        std::cout << "Index:\n";
        if (arrayIndex) arrayIndex->dump(indent + 2);
        printIndent(indent + 1);
        std::cout << "RHS:\n";
        if (value) value->dump(indent + 2);
    } else {
        std::cout << "AssignExpr(" << varName << " = ...)\n";
        if (value) value->dump(indent + 1);
    }
}

void CallExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void CallExpr::dump(int indent) const {
    printIndent(indent);
    std::cout << "CallExpr(" << callee << ") [" << inferredType.toString() << "]\n";
    for (const auto &arg : args) {
        if (arg) arg->dump(indent + 1);
    }
}

void BlockStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void BlockStmt::dump(int indent) const {
    printIndent(indent);
    std::cout << "BlockStmt {\n";
    for (const auto &stmt : statements) {
        if (stmt) stmt->dump(indent + 1);
    }
    printIndent(indent);
    std::cout << "}\n";
}

void VarDeclStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void VarDeclStmt::dump(int indent) const {
    printIndent(indent);
    std::cout << "VarDeclStmt(" << type.toString() << " " << name << ")\n";
    if (init) {
        printIndent(indent + 1);
        std::cout << "Init:\n";
        init->dump(indent + 2);
    }
}

void IfStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void IfStmt::dump(int indent) const {
    printIndent(indent);
    std::cout << "IfStmt\n";
    printIndent(indent + 1);
    std::cout << "Condition:\n";
    if (cond) cond->dump(indent + 2);
    printIndent(indent + 1);
    std::cout << "Then:\n";
    if (thenStmt) thenStmt->dump(indent + 2);
    if (elseStmt) {
        printIndent(indent + 1);
        std::cout << "Else:\n";
        elseStmt->dump(indent + 2);
    }
}

void WhileStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void WhileStmt::dump(int indent) const {
    printIndent(indent);
    std::cout << "WhileStmt\n";
    printIndent(indent + 1);
    std::cout << "Condition:\n";
    if (cond) cond->dump(indent + 2);
    printIndent(indent + 1);
    std::cout << "Body:\n";
    if (body) body->dump(indent + 2);
}

void ReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ReturnStmt::dump(int indent) const {
    printIndent(indent);
    std::cout << "ReturnStmt\n";
    if (value) value->dump(indent + 1);
}

void ExprStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ExprStmt::dump(int indent) const {
    printIndent(indent);
    std::cout << "ExprStmt\n";
    if (expr) expr->dump(indent + 1);
}

void PrototypeAST::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void PrototypeAST::dump(int indent) const {
    printIndent(indent);
    std::cout << "PrototypeAST(" << returnType.toString() << " " << name << "(";
    for (size_t i = 0; i < params.size(); ++i) {
        std::cout << params[i].type.toString() << " " << params[i].name;
        if (i + 1 < params.size()) std::cout << ", ";
    }
    std::cout << "))\n";
}

void FunctionAST::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void FunctionAST::dump(int indent) const {
    printIndent(indent);
    std::cout << "FunctionAST\n";
    if (proto) proto->dump(indent + 1);
    if (body) body->dump(indent + 1);
}

void ModuleAST::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ModuleAST::dump(int indent) const {
    std::cout << "ModuleAST(" << name << ")\n";
    for (const auto &fn : functions) {
        if (fn) fn->dump(indent + 1);
    }
}

} // namespace algoc

