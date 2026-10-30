#include "SemanticAnalyzer.h"
#include <sstream>

namespace algoc {

SemanticAnalyzer::SemanticAnalyzer() {
    registerBuiltins();
}

void SemanticAnalyzer::registerBuiltins() {
    // print_int(int x) -> void
    Symbol printIntSym;
    printIntSym.name = "print_int";
    printIntSym.type = {TypeKind::Void};
    printIntSym.kind = SymbolKind::Function;
    printIntSym.paramTypes = {{TypeKind::Int}};
    symTab.insert(printIntSym);

    // print_float(float x) -> void
    Symbol printFloatSym;
    printFloatSym.name = "print_float";
    printFloatSym.type = {TypeKind::Void};
    printFloatSym.kind = SymbolKind::Function;
    printFloatSym.paramTypes = {{TypeKind::Float}};
    symTab.insert(printFloatSym);

    // print_bool(bool x) -> void
    Symbol printBoolSym;
    printBoolSym.name = "print_bool";
    printBoolSym.type = {TypeKind::Void};
    printBoolSym.kind = SymbolKind::Function;
    printBoolSym.paramTypes = {{TypeKind::Bool}};
    symTab.insert(printBoolSym);

    // print_str(string s) -> void
    Symbol printStrSym;
    printStrSym.name = "print_str";
    printStrSym.type = {TypeKind::Void};
    printStrSym.kind = SymbolKind::Function;
    printStrSym.paramTypes = {{TypeKind::String}};
    symTab.insert(printStrSym);

    // println() -> void
    Symbol printlnSym;
    printlnSym.name = "println";
    printlnSym.type = {TypeKind::Void};
    printlnSym.kind = SymbolKind::Function;
    symTab.insert(printlnSym);

    // read_int() -> int
    Symbol readIntSym;
    readIntSym.name = "read_int";
    readIntSym.type = {TypeKind::Int};
    readIntSym.kind = SymbolKind::Function;
    symTab.insert(readIntSym);
}

void SemanticAnalyzer::reportSemanticError(const std::string &msg, const SourceLocation &loc) {
    std::ostringstream oss;
    oss << "Semantic Error at [" << loc.line << ":" << loc.column << "]: " << msg;
    errors.push_back(oss.str());
}

bool SemanticAnalyzer::isTypeCompatible(const TypeInfo &target, const TypeInfo &source) {
    if (target.kind == source.kind) {
        if (target.kind == TypeKind::Array) {
            return target.elementType == source.elementType;
        }
        return true;
    }
    // Allow int to float promotion
    if (target.kind == TypeKind::Float && source.kind == TypeKind::Int) {
        return true;
    }
    return false;
}

bool SemanticAnalyzer::checkDefiniteReturn(StmtAST *stmt) {
    if (!stmt) return false;

    if (isa<ReturnStmt>(stmt)) {
        return true;
    }

    if (auto block = dyn_cast<BlockStmt>(stmt)) {
        for (const auto &s : block->getStatements()) {
            if (checkDefiniteReturn(s.get())) {
                return true;
            }
        }
        return false;
    }

    if (auto ifStmt = dyn_cast<IfStmt>(stmt)) {
        if (ifStmt->getElseStmt() != nullptr) {
            return checkDefiniteReturn(ifStmt->getThenStmt()) &&
                   checkDefiniteReturn(ifStmt->getElseStmt());
        }
        return false;
    }

    return false;
}

bool SemanticAnalyzer::analyze(ModuleAST &module) {
    errors.clear();
    module.accept(*this);
    return errors.empty();
}

void SemanticAnalyzer::visit(ModuleAST &node) {
    // Phase 1: Register all function prototypes in the global symbol table
    for (const auto &fn : node.getFunctions()) {
        if (!fn || !fn->getProto()) continue;
        PrototypeAST *proto = fn->getProto();

        if (symTab.lookupLocal(proto->getName())) {
            reportSemanticError("Redefinition of function '" + proto->getName() + "'", proto->getLocation());
            continue;
        }

        Symbol fnSym;
        fnSym.name = proto->getName();
        fnSym.type = proto->getReturnType();
        fnSym.kind = SymbolKind::Function;
        fnSym.location = proto->getLocation();
        for (const auto &p : proto->getParams()) {
            fnSym.paramTypes.push_back(p.type);
        }
        symTab.insert(fnSym);
    }

    // Phase 2: Analyze function bodies
    for (const auto &fn : node.getFunctions()) {
        if (fn) fn->accept(*this);
    }
}

void SemanticAnalyzer::visit(PrototypeAST &node) {
    // Parameters are entered into the function's local scope
    for (const auto &param : node.getParams()) {
        if (symTab.lookupLocal(param.name)) {
            reportSemanticError("Duplicate parameter name '" + param.name + "' in function '" + currentFunctionName + "'", param.location);
        } else {
            Symbol sym;
            sym.name = param.name;
            sym.type = param.type;
            sym.kind = (param.type.kind == TypeKind::Array) ? SymbolKind::Array : SymbolKind::Parameter;
            sym.location = param.location;
            symTab.insert(sym);
        }
    }
}

void SemanticAnalyzer::visit(FunctionAST &node) {
    if (!node.getProto()) return;

    currentFunctionName = node.getProto()->getName();
    currentFunctionReturnType = node.getProto()->getReturnType();

    // Enter function level scope
    symTab.enterScope();

    node.getProto()->accept(*this);

    if (node.getBody()) {
        node.getBody()->accept(*this);

        // Definite return check for non-void functions
        if (currentFunctionReturnType.kind != TypeKind::Void) {
            bool hasReturn = checkDefiniteReturn(node.getBody());
            if (!hasReturn) {
                reportSemanticError("Control reaches end of non-void function '" + currentFunctionName +
                                    "' without return statement", node.getLocation());
            }
        }
    }

    symTab.exitScope();
}

void SemanticAnalyzer::visit(BlockStmt &node) {
    symTab.enterScope();
    for (const auto &stmt : node.getStatements()) {
        if (stmt) stmt->accept(*this);
    }
    symTab.exitScope();
}

void SemanticAnalyzer::visit(VarDeclStmt &node) {
    if (symTab.lookupLocal(node.getName())) {
        reportSemanticError("Redeclaration of variable '" + node.getName() + "' in the same scope", node.getLocation());
        return;
    }

    if (node.getInit()) {
        node.getInit()->accept(*this);
        TypeInfo initType = node.getInit()->getType();
        if (!isTypeCompatible(node.getType(), initType)) {
            reportSemanticError("Type mismatch in initialization of '" + node.getName() + "': declared as " +
                                node.getType().toString() + " but initialized with " + initType.toString(),
                                node.getLocation());
        }
    }

    Symbol sym;
    sym.name = node.getName();
    sym.type = node.getType();
    sym.kind = (node.getType().kind == TypeKind::Array) ? SymbolKind::Array : SymbolKind::Variable;
    sym.location = node.getLocation();
    symTab.insert(sym);
}

void SemanticAnalyzer::visit(IfStmt &node) {
    if (node.getCond()) {
        node.getCond()->accept(*this);
        TypeInfo condType = node.getCond()->getType();
        if (condType.kind != TypeKind::Bool && condType.kind != TypeKind::Int) {
            reportSemanticError("Condition in 'if' statement must evaluate to bool or int, found " +
                                condType.toString(), node.getLocation());
        }
    }

    if (node.getThenStmt()) {
        node.getThenStmt()->accept(*this);
    }

    if (node.getElseStmt()) {
        node.getElseStmt()->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStmt &node) {
    if (node.getCond()) {
        node.getCond()->accept(*this);
        TypeInfo condType = node.getCond()->getType();
        if (condType.kind != TypeKind::Bool && condType.kind != TypeKind::Int) {
            reportSemanticError("Condition in 'while' statement must evaluate to bool or int, found " +
                                condType.toString(), node.getLocation());
        }
    }

    if (node.getBody()) {
        node.getBody()->accept(*this);
    }
}

void SemanticAnalyzer::visit(ReturnStmt &node) {
    if (node.getValue()) {
        node.getValue()->accept(*this);
        TypeInfo valType = node.getValue()->getType();
        if (currentFunctionReturnType.kind == TypeKind::Void) {
            reportSemanticError("Void function '" + currentFunctionName + "' cannot return a value", node.getLocation());
        } else if (!isTypeCompatible(currentFunctionReturnType, valType)) {
            reportSemanticError("Function '" + currentFunctionName + "' returning " +
                                currentFunctionReturnType.toString() + " cannot return value of type " +
                                valType.toString(), node.getLocation());
        }
    } else {
        if (currentFunctionReturnType.kind != TypeKind::Void) {
            reportSemanticError("Non-void function '" + currentFunctionName + "' must return a value of type " +
                                currentFunctionReturnType.toString(), node.getLocation());
        }
    }
}

void SemanticAnalyzer::visit(ExprStmt &node) {
    if (node.getExpr()) {
        node.getExpr()->accept(*this);
    }
}

void SemanticAnalyzer::visit(NumberExpr &node) {
    // Type is already set in constructor
}

void SemanticAnalyzer::visit(BoolExpr &node) {
    // Type is already set in constructor
}

void SemanticAnalyzer::visit(StringLiteralExpr &node) {
    // Type is already set in constructor
}

void SemanticAnalyzer::visit(VariableExpr &node) {
    const Symbol *sym = symTab.lookup(node.getName());
    if (!sym) {
        reportSemanticError("Use of undeclared identifier '" + node.getName() + "'", node.getLocation());
        node.setType({TypeKind::Error});
        return;
    }
    node.setType(sym->type);
}

void SemanticAnalyzer::visit(ArrayAccessExpr &node) {
    const Symbol *sym = symTab.lookup(node.getName());
    if (!sym) {
        reportSemanticError("Use of undeclared array '" + node.getName() + "'", node.getLocation());
        node.setType({TypeKind::Error});
        return;
    }

    if (sym->type.kind != TypeKind::Array) {
        reportSemanticError("Subscripted object '" + node.getName() + "' is not an array", node.getLocation());
        node.setType({TypeKind::Error});
        return;
    }

    if (node.getIndex()) {
        node.getIndex()->accept(*this);
        if (node.getIndex()->getType().kind != TypeKind::Int) {
            reportSemanticError("Array index must be of integer type, found " +
                                node.getIndex()->getType().toString(), node.getLocation());
        }
    }

    // Element type
    node.setType({sym->type.elementType});
}

void SemanticAnalyzer::visit(UnaryExpr &node) {
    if (!node.getOperand()) return;
    node.getOperand()->accept(*this);
    TypeInfo opType = node.getOperand()->getType();

    if (node.getOp() == '-') {
        if (!opType.isNumeric()) {
            reportSemanticError("Unary minus operator requires numeric operand, found " + opType.toString(), node.getLocation());
            node.setType({TypeKind::Error});
        } else {
            node.setType(opType);
        }
    } else if (node.getOp() == '!') {
        if (opType.kind != TypeKind::Bool && opType.kind != TypeKind::Int) {
            reportSemanticError("Unary logical NOT operator requires bool operand, found " + opType.toString(), node.getLocation());
            node.setType({TypeKind::Error});
        } else {
            node.setType({TypeKind::Bool});
        }
    }
}

void SemanticAnalyzer::visit(BinaryExpr &node) {
    if (!node.getLHS() || !node.getRHS()) return;

    node.getLHS()->accept(*this);
    node.getRHS()->accept(*this);

    TypeInfo lType = node.getLHS()->getType();
    TypeInfo rType = node.getRHS()->getType();

    if (lType.kind == TypeKind::Error || rType.kind == TypeKind::Error) {
        node.setType({TypeKind::Error});
        return;
    }

    int op = node.getOp();

    // 1. Arithmetic operators: +, -, *, /, %
    if (op == '+' || op == '-' || op == '*' || op == '/' || op == '%') {
        if (!lType.isNumeric() || !rType.isNumeric()) {
            reportSemanticError("Arithmetic operator requires numeric operands, found " +
                                lType.toString() + " and " + rType.toString(), node.getLocation());
            node.setType({TypeKind::Error});
            return;
        }
        if (op == '%' && (lType.kind != TypeKind::Int || rType.kind != TypeKind::Int)) {
            reportSemanticError("Modulo operator '%' is only valid on integers", node.getLocation());
            node.setType({TypeKind::Error});
            return;
        }

        if (lType.kind == TypeKind::Float || rType.kind == TypeKind::Float) {
            node.setType({TypeKind::Float});
        } else {
            node.setType({TypeKind::Int});
        }
        return;
    }

    // 2. Relational operators: <, <=, >, >=
    if (op == '<' || op == '>' || op == tok_le || op == tok_ge) {
        if (!lType.isNumeric() || !rType.isNumeric()) {
            reportSemanticError("Relational comparison requires numeric operands, found " +
                                lType.toString() + " and " + rType.toString(), node.getLocation());
            node.setType({TypeKind::Error});
            return;
        }
        node.setType({TypeKind::Bool});
        return;
    }

    // 3. Equality operators: ==, !=
    if (op == tok_eq || op == tok_ne) {
        if (!isTypeCompatible(lType, rType) && !isTypeCompatible(rType, lType)) {
            reportSemanticError("Comparison between incompatible types " + lType.toString() +
                                " and " + rType.toString(), node.getLocation());
            node.setType({TypeKind::Error});
            return;
        }
        node.setType({TypeKind::Bool});
        return;
    }

    // 4. Logical operators: &&, ||
    if (op == tok_and || op == tok_or) {
        if ((lType.kind != TypeKind::Bool && lType.kind != TypeKind::Int) ||
            (rType.kind != TypeKind::Bool && rType.kind != TypeKind::Int)) {
            reportSemanticError("Logical operator requires boolean operands, found " +
                                lType.toString() + " and " + rType.toString(), node.getLocation());
            node.setType({TypeKind::Error});
            return;
        }
        node.setType({TypeKind::Bool});
        return;
    }
}

void SemanticAnalyzer::visit(AssignExpr &node) {
    const Symbol *sym = symTab.lookup(node.getVarName());
    if (!sym) {
        reportSemanticError("Assignment to undeclared identifier '" + node.getVarName() + "'", node.getLocation());
        node.setType({TypeKind::Error});
        return;
    }

    if (node.getIsArrayAssign()) {
        if (sym->type.kind != TypeKind::Array) {
            reportSemanticError("Array assignment target '" + node.getVarName() + "' is not an array", node.getLocation());
            node.setType({TypeKind::Error});
            return;
        }

        if (node.getArrayIndex()) {
            node.getArrayIndex()->accept(*this);
            if (node.getArrayIndex()->getType().kind != TypeKind::Int) {
                reportSemanticError("Array index must be of integer type, found " +
                                    node.getArrayIndex()->getType().toString(), node.getLocation());
            }
        }

        if (node.getValue()) {
            node.getValue()->accept(*this);
            TypeInfo valType = node.getValue()->getType();
            TypeInfo elemType{sym->type.elementType};
            if (!isTypeCompatible(elemType, valType)) {
                reportSemanticError("Type mismatch in array assignment: array elements are " +
                                    elemType.toString() + " but assigning " + valType.toString(),
                                    node.getLocation());
            }
        }
        node.setType({sym->type.elementType});
    } else {
        if (sym->type.kind == TypeKind::Array) {
            reportSemanticError("Cannot assign directly to entire array '" + node.getVarName() + "'", node.getLocation());
            node.setType({TypeKind::Error});
            return;
        }

        if (node.getValue()) {
            node.getValue()->accept(*this);
            TypeInfo valType = node.getValue()->getType();
            if (!isTypeCompatible(sym->type, valType)) {
                reportSemanticError("Type mismatch in assignment to '" + node.getVarName() +
                                    "': variable is " + sym->type.toString() + " but assigned value is " +
                                    valType.toString(), node.getLocation());
            }
        }
        node.setType(sym->type);
    }
}

void SemanticAnalyzer::visit(CallExpr &node) {
    const Symbol *sym = symTab.lookup(node.getCallee());
    if (!sym) {
        reportSemanticError("Call to undeclared function '" + node.getCallee() + "'", node.getLocation());
        node.setType({TypeKind::Error});
        return;
    }

    if (sym->kind != SymbolKind::Function) {
        reportSemanticError("Identifier '" + node.getCallee() + "' is not a function", node.getLocation());
        node.setType({TypeKind::Error});
        return;
    }

    if (node.getArgs().size() != sym->paramTypes.size()) {
        reportSemanticError("Function '" + node.getCallee() + "' expects " +
                            std::to_string(sym->paramTypes.size()) + " arguments, but got " +
                            std::to_string(node.getArgs().size()), node.getLocation());
        node.setType(sym->type);
        return;
    }

    for (size_t i = 0; i < node.getArgs().size(); ++i) {
        auto &arg = node.getArgs()[i];
        if (arg) {
            arg->accept(*this);
            TypeInfo argType = arg->getType();
            TypeInfo expectedType = sym->paramTypes[i];
            if (!isTypeCompatible(expectedType, argType)) {
                reportSemanticError("Argument " + std::to_string(i + 1) + " of function '" +
                                    node.getCallee() + "' expects type " + expectedType.toString() +
                                    ", but got " + argType.toString(), node.getLocation());
            }
        }
    }

    node.setType(sym->type);
}

} // namespace algoc

