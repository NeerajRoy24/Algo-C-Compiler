#include "VM.h"
#include <iostream>

namespace algoc {

VMCompiler::VMCompiler() = default;

void VMCompiler::emit(OpCode op, int64_t i, double f, const std::string &s) {
    if (currentFn) {
        currentFn->code.push_back({op, i, f, s});
    }
}

int VMCompiler::emitJump(OpCode op) {
    if (!currentFn) return -1;
    currentFn->code.push_back({op, -1, 0.0, ""});
    return (int)currentFn->code.size() - 1;
}

void VMCompiler::patchJump(int instructionIndex) {
    if (currentFn && instructionIndex >= 0 && instructionIndex < (int)currentFn->code.size()) {
        currentFn->code[instructionIndex].intOperand = (int64_t)currentFn->code.size();
    }
}

std::unordered_map<std::string, CompiledFunction> VMCompiler::compile(ModuleAST &module) {
    compiledFunctions.clear();
    module.accept(*this);
    return compiledFunctions;
}

void VMCompiler::visit(ModuleAST &node) {
    for (const auto &fn : node.getFunctions()) {
        if (fn) fn->accept(*this);
    }
}

void VMCompiler::visit(PrototypeAST &node) {
    CompiledFunction fn;
    fn.name = node.getName();
    fn.numParams = (int)node.getParams().size();
    compiledFunctions[node.getName()] = fn;
    currentFn = &compiledFunctions[node.getName()];

    nextLocalSlot = 0;
    for (const auto &p : node.getParams()) {
        currentFn->localSlots[p.name] = nextLocalSlot++;
    }
}

void VMCompiler::visit(FunctionAST &node) {
    if (!node.getProto()) return;
    node.getProto()->accept(*this);

    if (node.getBody()) {
        node.getBody()->accept(*this);
    }

    emit(OP_CONST_INT, 0);
    emit(OP_RET);
}

void VMCompiler::visit(BlockStmt &node) {
    for (const auto &stmt : node.getStatements()) {
        if (stmt) stmt->accept(*this);
    }
}

void VMCompiler::visit(VarDeclStmt &node) {
    int slot = nextLocalSlot++;
    currentFn->localSlots[node.getName()] = slot;

    if (node.getType().kind == TypeKind::Array) {
        emit(OP_ARRAY_ALLOC, node.getType().arraySize);
        emit(OP_STORE_LOCAL, slot);
    } else if (node.getInit()) {
        node.getInit()->accept(*this);
        emit(OP_STORE_LOCAL, slot);
    }
}

void VMCompiler::visit(IfStmt &node) {
    node.getCond()->accept(*this);
    int jumpFalse = emitJump(OP_JUMP_IF_FALSE);

    if (node.getThenStmt()) node.getThenStmt()->accept(*this);

    if (node.getElseStmt()) {
        int jumpEnd = emitJump(OP_JUMP);
        patchJump(jumpFalse);
        node.getElseStmt()->accept(*this);
        patchJump(jumpEnd);
    } else {
        patchJump(jumpFalse);
    }
}

void VMCompiler::visit(WhileStmt &node) {
    int loopStart = (int)currentFn->code.size();
    node.getCond()->accept(*this);

    int jumpExit = emitJump(OP_JUMP_IF_FALSE);
    if (node.getBody()) node.getBody()->accept(*this);

    emit(OP_JUMP, loopStart);
    patchJump(jumpExit);
}

void VMCompiler::visit(ReturnStmt &node) {
    if (node.getValue()) {
        node.getValue()->accept(*this);
    } else {
        emit(OP_CONST_INT, 0);
    }
    emit(OP_RET);
}

void VMCompiler::visit(ExprStmt &node) {
    if (node.getExpr()) {
        node.getExpr()->accept(*this);
        emit(OP_POP);
    }
}

void VMCompiler::visit(NumberExpr &node) {
    if (node.getIsFloat()) {
        emit(OP_CONST_FLOAT, 0, node.getFloatVal());
    } else {
        emit(OP_CONST_INT, node.getIntVal());
    }
}

void VMCompiler::visit(BoolExpr &node) {
    emit(OP_CONST_BOOL, node.getValue() ? 1 : 0);
}

void VMCompiler::visit(StringLiteralExpr &node) {
    emit(OP_CONST_STR, 0, 0.0, node.getValue());
}

void VMCompiler::visit(VariableExpr &node) {
    auto it = currentFn->localSlots.find(node.getName());
    if (it != currentFn->localSlots.end()) {
        emit(OP_LOAD_LOCAL, it->second);
    }
}

void VMCompiler::visit(ArrayAccessExpr &node) {
    auto it = currentFn->localSlots.find(node.getName());
    if (it != currentFn->localSlots.end()) {
        emit(OP_LOAD_LOCAL, it->second); // pushes array handle
        node.getIndex()->accept(*this);   // pushes index
        emit(OP_ARRAY_LOAD);
    }
}

void VMCompiler::visit(UnaryExpr &node) {
    node.getOperand()->accept(*this);
    if (node.getOp() == '-') emit(OP_NEG);
    else if (node.getOp() == '!') emit(OP_NOT);
}

void VMCompiler::visit(BinaryExpr &node) {
    node.getLHS()->accept(*this);
    node.getRHS()->accept(*this);

    switch (node.getOp()) {
        case '+': emit(OP_ADD); break;
        case '-': emit(OP_SUB); break;
        case '*': emit(OP_MUL); break;
        case '/': emit(OP_DIV); break;
        case '%': emit(OP_MOD); break;
        case '<': emit(OP_LT); break;
        case '>': emit(OP_GT); break;
        case tok_le: emit(OP_LE); break;
        case tok_ge: emit(OP_GE); break;
        case tok_eq: emit(OP_EQ); break;
        case tok_ne: emit(OP_NE); break;
        case tok_and: emit(OP_AND); break;
        case tok_or: emit(OP_OR); break;
    }
}

void VMCompiler::visit(AssignExpr &node) {
    auto it = currentFn->localSlots.find(node.getVarName());
    if (it != currentFn->localSlots.end()) {
        if (node.getIsArrayAssign()) {
            emit(OP_LOAD_LOCAL, it->second); // array
            node.getArrayIndex()->accept(*this); // index
            node.getValue()->accept(*this); // value
            emit(OP_ARRAY_STORE);
        } else {
            node.getValue()->accept(*this);
            emit(OP_STORE_LOCAL, it->second);
            emit(OP_LOAD_LOCAL, it->second);
        }
    }
}

void VMCompiler::visit(CallExpr &node) {
    for (const auto &arg : node.getArgs()) {
        arg->accept(*this);
    }

    if (node.getCallee() == "print_int") {
        emit(OP_PRINT_INT);
        emit(OP_CONST_INT, 0); // dummy return
    } else if (node.getCallee() == "print_float") {
        emit(OP_PRINT_FLOAT);
        emit(OP_CONST_INT, 0);
    } else if (node.getCallee() == "print_bool") {
        emit(OP_PRINT_BOOL);
        emit(OP_CONST_INT, 0);
    } else if (node.getCallee() == "print_str") {
        emit(OP_PRINT_STR);
        emit(OP_CONST_INT, 0);
    } else if (node.getCallee() == "println") {
        emit(OP_PRINTLN);
        emit(OP_CONST_INT, 0);
    } else {
        emit(OP_CALL, (int64_t)node.getArgs().size(), 0.0, node.getCallee());
    }
}

// ----------------- Virtual Machine Execution -----------------

struct CallFrame {
    std::string fnName;
    int ip = 0;
    std::vector<Value> locals;
};

int VirtualMachine::run(const std::unordered_map<std::string, CompiledFunction> &functions,
                        const std::string &entryPoint) {
    auto it = functions.find(entryPoint);
    if (it == functions.end()) {
        std::cerr << "Runtime Error: Entry point '" << entryPoint << "' not found!\n";
        return -1;
    }

    std::vector<CallFrame> callStack;
    CallFrame mainFrame;
    mainFrame.fnName = entryPoint;
    mainFrame.ip = 0;
    mainFrame.locals.resize(32, int64_t(0));
    callStack.push_back(mainFrame);

    std::vector<Value> stack;

    while (!callStack.empty()) {
        CallFrame &frame = callStack.back();
        const CompiledFunction &curFn = functions.at(frame.fnName);

        if (frame.ip >= (int)curFn.code.size()) {
            callStack.pop_back();
            continue;
        }

        const Instruction &inst = curFn.code[frame.ip++];

        switch (inst.op) {
            case OP_CONST_INT:
                stack.push_back(inst.intOperand);
                break;
            case OP_CONST_FLOAT:
                stack.push_back(inst.floatOperand);
                break;
            case OP_CONST_BOOL:
                stack.push_back(inst.intOperand != 0);
                break;
            case OP_CONST_STR:
                stack.push_back(inst.strOperand);
                break;

            case OP_LOAD_LOCAL: {
                int slot = (int)inst.intOperand;
                if (slot < (int)frame.locals.size()) {
                    stack.push_back(frame.locals[slot]);
                } else {
                    stack.push_back(int64_t(0));
                }
                break;
            }
            case OP_STORE_LOCAL: {
                int slot = (int)inst.intOperand;
                if (!stack.empty()) {
                    Value v = stack.back();
                    stack.pop_back();
                    if (slot >= (int)frame.locals.size()) {
                        frame.locals.resize(slot + 1);
                    }
                    frame.locals[slot] = v;
                }
                break;
            }

            case OP_ARRAY_ALLOC: {
                int size = (int)inst.intOperand;
                auto arr = std::make_shared<std::vector<int64_t>>(size, 0);
                stack.push_back(arr);
                break;
            }
            case OP_ARRAY_LOAD: {
                int64_t idx = std::get<int64_t>(stack.back()); stack.pop_back();
                auto arr = std::get<std::shared_ptr<std::vector<int64_t>>>(stack.back()); stack.pop_back();
                if (arr && idx >= 0 && idx < (int64_t)arr->size()) {
                    stack.push_back((*arr)[idx]);
                } else {
                    stack.push_back(int64_t(0));
                }
                break;
            }
            case OP_ARRAY_STORE: {
                int64_t val = std::get<int64_t>(stack.back()); stack.pop_back();
                int64_t idx = std::get<int64_t>(stack.back()); stack.pop_back();
                auto arr = std::get<std::shared_ptr<std::vector<int64_t>>>(stack.back()); stack.pop_back();
                if (arr && idx >= 0 && idx < (int64_t)arr->size()) {
                    (*arr)[idx] = val;
                }
                stack.push_back(val);
                break;
            }

            case OP_ADD: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                if (std::holds_alternative<int64_t>(l) && std::holds_alternative<int64_t>(r)) {
                    stack.push_back(std::get<int64_t>(l) + std::get<int64_t>(r));
                } else {
                    double dl = std::holds_alternative<double>(l) ? std::get<double>(l) : std::get<int64_t>(l);
                    double dr = std::holds_alternative<double>(r) ? std::get<double>(r) : std::get<int64_t>(r);
                    stack.push_back(dl + dr);
                }
                break;
            }
            case OP_SUB: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                if (std::holds_alternative<int64_t>(l) && std::holds_alternative<int64_t>(r)) {
                    stack.push_back(std::get<int64_t>(l) - std::get<int64_t>(r));
                } else {
                    double dl = std::holds_alternative<double>(l) ? std::get<double>(l) : std::get<int64_t>(l);
                    double dr = std::holds_alternative<double>(r) ? std::get<double>(r) : std::get<int64_t>(r);
                    stack.push_back(dl - dr);
                }
                break;
            }
            case OP_MUL: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                if (std::holds_alternative<int64_t>(l) && std::holds_alternative<int64_t>(r)) {
                    stack.push_back(std::get<int64_t>(l) * std::get<int64_t>(r));
                } else {
                    double dl = std::holds_alternative<double>(l) ? std::get<double>(l) : std::get<int64_t>(l);
                    double dr = std::holds_alternative<double>(r) ? std::get<double>(r) : std::get<int64_t>(r);
                    stack.push_back(dl * dr);
                }
                break;
            }
            case OP_DIV: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                if (std::holds_alternative<int64_t>(l) && std::holds_alternative<int64_t>(r)) {
                    int64_t div = std::get<int64_t>(r);
                    stack.push_back(div != 0 ? std::get<int64_t>(l) / div : 0);
                } else {
                    double dl = std::holds_alternative<double>(l) ? std::get<double>(l) : std::get<int64_t>(l);
                    double dr = std::holds_alternative<double>(r) ? std::get<double>(r) : std::get<int64_t>(r);
                    stack.push_back(dr != 0.0 ? dl / dr : 0.0);
                }
                break;
            }
            case OP_MOD: {
                int64_t r = std::get<int64_t>(stack.back()); stack.pop_back();
                int64_t l = std::get<int64_t>(stack.back()); stack.pop_back();
                stack.push_back(r != 0 ? l % r : 0);
                break;
            }

            case OP_LT: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                if (std::holds_alternative<int64_t>(l) && std::holds_alternative<int64_t>(r)) {
                    stack.push_back(std::get<int64_t>(l) < std::get<int64_t>(r));
                } else {
                    double dl = std::holds_alternative<double>(l) ? std::get<double>(l) : std::get<int64_t>(l);
                    double dr = std::holds_alternative<double>(r) ? std::get<double>(r) : std::get<int64_t>(r);
                    stack.push_back(dl < dr);
                }
                break;
            }
            case OP_GT: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                if (std::holds_alternative<int64_t>(l) && std::holds_alternative<int64_t>(r)) {
                    stack.push_back(std::get<int64_t>(l) > std::get<int64_t>(r));
                } else {
                    double dl = std::holds_alternative<double>(l) ? std::get<double>(l) : std::get<int64_t>(l);
                    double dr = std::holds_alternative<double>(r) ? std::get<double>(r) : std::get<int64_t>(r);
                    stack.push_back(dl > dr);
                }
                break;
            }
            case OP_LE: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                if (std::holds_alternative<int64_t>(l) && std::holds_alternative<int64_t>(r)) {
                    stack.push_back(std::get<int64_t>(l) <= std::get<int64_t>(r));
                }
                break;
            }
            case OP_GE: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                if (std::holds_alternative<int64_t>(l) && std::holds_alternative<int64_t>(r)) {
                    stack.push_back(std::get<int64_t>(l) >= std::get<int64_t>(r));
                }
                break;
            }
            case OP_EQ: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                stack.push_back(l == r);
                break;
            }
            case OP_NE: {
                auto r = stack.back(); stack.pop_back();
                auto l = stack.back(); stack.pop_back();
                stack.push_back(l != r);
                break;
            }

            case OP_AND: {
                bool r = std::holds_alternative<bool>(stack.back()) ? std::get<bool>(stack.back()) : (std::get<int64_t>(stack.back()) != 0);
                stack.pop_back();
                bool l = std::holds_alternative<bool>(stack.back()) ? std::get<bool>(stack.back()) : (std::get<int64_t>(stack.back()) != 0);
                stack.pop_back();
                stack.push_back(l && r);
                break;
            }
            case OP_OR: {
                bool r = std::holds_alternative<bool>(stack.back()) ? std::get<bool>(stack.back()) : (std::get<int64_t>(stack.back()) != 0);
                stack.pop_back();
                bool l = std::holds_alternative<bool>(stack.back()) ? std::get<bool>(stack.back()) : (std::get<int64_t>(stack.back()) != 0);
                stack.pop_back();
                stack.push_back(l || r);
                break;
            }
            case OP_NEG: {
                if (std::holds_alternative<int64_t>(stack.back())) {
                    int64_t v = std::get<int64_t>(stack.back()); stack.pop_back();
                    stack.push_back(-v);
                } else if (std::holds_alternative<double>(stack.back())) {
                    double v = std::get<double>(stack.back()); stack.pop_back();
                    stack.push_back(-v);
                }
                break;
            }
            case OP_NOT: {
                bool b = std::holds_alternative<bool>(stack.back()) ? std::get<bool>(stack.back()) : (std::get<int64_t>(stack.back()) != 0);
                stack.pop_back();
                stack.push_back(!b);
                break;
            }

            case OP_JUMP:
                frame.ip = (int)inst.intOperand;
                break;
            case OP_JUMP_IF_FALSE: {
                bool cond = true;
                if (std::holds_alternative<bool>(stack.back())) cond = std::get<bool>(stack.back());
                else if (std::holds_alternative<int64_t>(stack.back())) cond = (std::get<int64_t>(stack.back()) != 0);
                stack.pop_back();
                if (!cond) {
                    frame.ip = (int)inst.intOperand;
                }
                break;
            }

            case OP_CALL: {
                int numArgs = (int)inst.intOperand;
                std::string targetFn = inst.strOperand;
                auto fnIt = functions.find(targetFn);
                if (fnIt == functions.end()) {
                    std::cerr << "Runtime Error: Function '" << targetFn << "' not found!\n";
                    return -1;
                }

                CallFrame newFrame;
                newFrame.fnName = targetFn;
                newFrame.ip = 0;
                newFrame.locals.resize(32, int64_t(0));

                for (int a = numArgs - 1; a >= 0; --a) {
                    newFrame.locals[a] = stack.back();
                    stack.pop_back();
                }

                callStack.push_back(newFrame);
                break;
            }

            case OP_RET: {
                Value retVal = int64_t(0);
                if (!stack.empty()) {
                    retVal = stack.back();
                    stack.pop_back();
                }
                callStack.pop_back();
                if (callStack.empty()) {
                    if (std::holds_alternative<int64_t>(retVal)) {
                        return (int)std::get<int64_t>(retVal);
                    }
                    return 0;
                }
                stack.push_back(retVal);
                break;
            }

            case OP_PRINT_INT: {
                int64_t val = std::get<int64_t>(stack.back());
                stack.pop_back();
                std::cout << val;
                break;
            }
            case OP_PRINT_FLOAT: {
                double val = std::get<double>(stack.back());
                stack.pop_back();
                std::cout << val;
                break;
            }
            case OP_PRINT_BOOL: {
                bool val = std::get<bool>(stack.back());
                stack.pop_back();
                std::cout << (val ? "true" : "false");
                break;
            }
            case OP_PRINT_STR: {
                std::string val = std::get<std::string>(stack.back());
                stack.pop_back();
                std::cout << val;
                break;
            }
            case OP_PRINTLN: {
                std::cout << "\n";
                break;
            }
            case OP_READ_INT: {
                int64_t val = 0;
                std::cin >> val;
                stack.push_back(val);
                break;
            }

            case OP_POP:
                if (!stack.empty()) stack.pop_back();
                break;
            case OP_HALT:
                return 0;
        }
    }

    return 0;
}

} // namespace algoc
