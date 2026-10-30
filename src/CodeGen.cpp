#include "CodeGen.h"
#include <iostream>

namespace algoc {

CodeGen::CodeGen() = default;

std::string CodeGen::newRegister() {
    return "%t" + std::to_string(++tempRegisterCount);
}

std::string CodeGen::newLabel(const std::string &prefix) {
    return prefix + "." + std::to_string(++labelCount);
}

void CodeGen::enterScope() {
    namedValuesScope.emplace_back();
}

void CodeGen::exitScope() {
    if (!namedValuesScope.empty()) {
        namedValuesScope.pop_back();
    }
}

void CodeGen::setVariable(const std::string &name, const LLVMValue &val) {
    if (namedValuesScope.empty()) namedValuesScope.emplace_back();
    namedValuesScope.back()[name] = val;
}

LLVMValue CodeGen::getVariable(const std::string &name) {
    for (auto it = namedValuesScope.rbegin(); it != namedValuesScope.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return LLVMValue{"", {TypeKind::Void}, false};
}

void CodeGen::emitInstruction(const std::string &inst) {
    BasicBlock *bb = getCurrentBlock();
    if (bb && !bb->hasTerminator) {
        bb->instructions.push_back("  " + inst);
    }
}

void CodeGen::createBlock(const std::string &label) {
    if (currentFunctionIndex >= 0 && currentFunctionIndex < (int)functions.size()) {
        BasicBlock bb;
        bb.label = label;
        bb.hasTerminator = false;
        functions[currentFunctionIndex].blocks.push_back(bb);
        functions[currentFunctionIndex].currentBlockIndex = (int)functions[currentFunctionIndex].blocks.size() - 1;
    }
}

void CodeGen::switchToBlock(const std::string &label) {
    if (currentFunctionIndex >= 0 && currentFunctionIndex < (int)functions.size()) {
        auto &blocks = functions[currentFunctionIndex].blocks;
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i].label == label) {
                functions[currentFunctionIndex].currentBlockIndex = (int)i;
                return;
            }
        }
        createBlock(label);
    }
}

BasicBlock *CodeGen::getCurrentBlock() {
    if (currentFunctionIndex >= 0 && currentFunctionIndex < (int)functions.size()) {
        auto &fn = functions[currentFunctionIndex];
        if (fn.currentBlockIndex >= 0 && fn.currentBlockIndex < (int)fn.blocks.size()) {
            return &fn.blocks[fn.currentBlockIndex];
        }
    }
    return nullptr;
}

std::string CodeGen::llvmType(const TypeInfo &type) {
    switch (type.kind) {
        case TypeKind::Int: return "i32";
        case TypeKind::Float: return "double";
        case TypeKind::Bool: return "i1";
        case TypeKind::Void: return "void";
        case TypeKind::String: return "i8*";
        case TypeKind::Array: {
            std::string elem = (type.elementType == TypeKind::Float) ? "double" : "i32";
            if (type.arraySize > 0) {
                return "[" + std::to_string(type.arraySize) + " x " + elem + "]";
            }
            return elem + "*";
        }
        default: return "i32";
    }
}

std::string CodeGen::generateIR(ModuleAST &module) {
    moduleStream.str("");
    moduleStream.clear();
    functions.clear();
    stringConstants.clear();
    tempRegisterCount = 0;
    labelCount = 0;
    stringLiteralCount = 0;

    module.accept(*this);

    // Module header and target triple
    moduleStream << "; ModuleID = '" << module.getName() << "'\n";
    moduleStream << "source_filename = \"" << module.getName() << ".algoc\"\n";
    moduleStream << "target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n";
    moduleStream << "target triple = \"x86_64-pc-windows-msvc\"\n\n";

    // String literals table
    for (const auto &strConst : stringConstants) {
        moduleStream << strConst.first << " = private unnamed_addr constant ["
                     << strConst.second.size() + 1 << " x i8] c\""
                     << strConst.second << "\\00\", align 1\n";
    }
    if (!stringConstants.empty()) moduleStream << "\n";

    // Standard builtin declarations
    moduleStream << "; Algo-C Standard Builtin Declarations\n";
    moduleStream << "declare void @print_int(i32)\n";
    moduleStream << "declare void @print_float(double)\n";
    moduleStream << "declare void @print_bool(i1)\n";
    moduleStream << "declare void @print_str(i8*)\n";
    moduleStream << "declare void @println()\n";
    moduleStream << "declare i32 @read_int()\n\n";

    // Functions
    for (const auto &fn : functions) {
        moduleStream << "define " << llvmType(fn.returnType) << " @" << fn.name << "(";
        for (size_t i = 0; i < fn.params.size(); ++i) {
            moduleStream << llvmType(fn.params[i].type) << " %" << fn.params[i].name;
            if (i + 1 < fn.params.size()) moduleStream << ", ";
        }
        moduleStream << ") {\n";

        for (const auto &bb : fn.blocks) {
            moduleStream << bb.label << ":\n";
            for (const auto &inst : bb.instructions) {
                moduleStream << inst << "\n";
            }
        }
        moduleStream << "}\n\n";
    }

    return moduleStream.str();
}

void CodeGen::visit(ModuleAST &node) {
    for (const auto &fn : node.getFunctions()) {
        if (fn) fn->accept(*this);
    }
}

void CodeGen::visit(PrototypeAST &node) {
    FunctionIR fnIR;
    fnIR.name = node.getName();
    fnIR.returnType = node.getReturnType();
    fnIR.params = node.getParams();
    functions.push_back(fnIR);
    currentFunctionIndex = (int)functions.size() - 1;

    createBlock("entry");

    // Alloca and store each argument in entry block
    enterScope();
    for (const auto &param : node.getParams()) {
        std::string allocaReg = "%" + param.name + ".addr";
        std::string ty = llvmType(param.type);
        emitInstruction(allocaReg + " = alloca " + ty);
        emitInstruction("store " + ty + " %" + param.name + ", " + ty + "* " + allocaReg);
        setVariable(param.name, {allocaReg, param.type, true});
    }
}

void CodeGen::visit(FunctionAST &node) {
    if (!node.getProto()) return;
    node.getProto()->accept(*this);

    if (node.getBody()) {
        node.getBody()->accept(*this);
    }

    // Ensure the last block has a terminator
    BasicBlock *bb = getCurrentBlock();
    if (bb && !bb->hasTerminator) {
        if (node.getProto()->getReturnType().kind == TypeKind::Void) {
            emitInstruction("ret void");
        } else if (node.getProto()->getReturnType().kind == TypeKind::Int) {
            emitInstruction("ret i32 0");
        } else if (node.getProto()->getReturnType().kind == TypeKind::Float) {
            emitInstruction("ret double 0.0");
        } else if (node.getProto()->getReturnType().kind == TypeKind::Bool) {
            emitInstruction("ret i1 0");
        }
        bb->hasTerminator = true;
    }

    exitScope();
}

void CodeGen::visit(BlockStmt &node) {
    enterScope();
    for (const auto &stmt : node.getStatements()) {
        if (stmt) stmt->accept(*this);
    }
    exitScope();
}

void CodeGen::visit(VarDeclStmt &node) {
    std::string allocaReg = "%" + node.getName() + ".slot";
    std::string ty = llvmType(node.getType());

    emitInstruction(allocaReg + " = alloca " + ty);
    setVariable(node.getName(), {allocaReg, node.getType(), true});

    if (node.getInit()) {
        node.getInit()->accept(*this);
        LLVMValue initVal = lastValue;
        emitInstruction("store " + llvmType(node.getType()) + " " + initVal.name + ", " + ty + "* " + allocaReg);
    }
}

void CodeGen::visit(IfStmt &node) {
    std::string thenBB = newLabel("then");
    std::string elseBB = newLabel("else");
    std::string mergeBB = newLabel("ifmerge");

    node.getCond()->accept(*this);
    LLVMValue condVal = lastValue;

    // Convert condition to i1 if not already
    std::string boolCond = condVal.name;
    if (condVal.type.kind == TypeKind::Int) {
        std::string cmpReg = newRegister();
        emitInstruction(cmpReg + " = icmp ne i32 " + condVal.name + ", 0");
        boolCond = cmpReg;
    }

    bool hasElse = (node.getElseStmt() != nullptr);
    std::string falseTarget = hasElse ? elseBB : mergeBB;

    emitInstruction("br i1 " + boolCond + ", label %" + thenBB + ", label %" + falseTarget);
    BasicBlock *cur = getCurrentBlock();
    if (cur) cur->hasTerminator = true;

    // Emit 'then' block
    createBlock(thenBB);
    if (node.getThenStmt()) node.getThenStmt()->accept(*this);
    cur = getCurrentBlock();
    if (cur && !cur->hasTerminator) {
        emitInstruction("br label %" + mergeBB);
        cur->hasTerminator = true;
    }

    // Emit 'else' block if present
    if (hasElse) {
        createBlock(elseBB);
        node.getElseStmt()->accept(*this);
        cur = getCurrentBlock();
        if (cur && !cur->hasTerminator) {
            emitInstruction("br label %" + mergeBB);
            cur->hasTerminator = true;
        }
    }

    // Merge block
    createBlock(mergeBB);
}

void CodeGen::visit(WhileStmt &node) {
    std::string condBB = newLabel("whilecond");
    std::string bodyBB = newLabel("whilebody");
    std::string afterBB = newLabel("whileafter");

    emitInstruction("br label %" + condBB);
    BasicBlock *cur = getCurrentBlock();
    if (cur) cur->hasTerminator = true;

    // Condition block
    createBlock(condBB);
    node.getCond()->accept(*this);
    LLVMValue condVal = lastValue;

    std::string boolCond = condVal.name;
    if (condVal.type.kind == TypeKind::Int) {
        std::string cmpReg = newRegister();
        emitInstruction(cmpReg + " = icmp ne i32 " + condVal.name + ", 0");
        boolCond = cmpReg;
    }

    emitInstruction("br i1 " + boolCond + ", label %" + bodyBB + ", label %" + afterBB);
    cur = getCurrentBlock();
    if (cur) cur->hasTerminator = true;

    // Body block
    createBlock(bodyBB);
    if (node.getBody()) node.getBody()->accept(*this);
    cur = getCurrentBlock();
    if (cur && !cur->hasTerminator) {
        emitInstruction("br label %" + condBB);
        cur->hasTerminator = true;
    }

    // After block
    createBlock(afterBB);
}

void CodeGen::visit(ReturnStmt &node) {
    if (node.getValue()) {
        node.getValue()->accept(*this);
        emitInstruction("ret " + llvmType(lastValue.type) + " " + lastValue.name);
    } else {
        emitInstruction("ret void");
    }
    BasicBlock *cur = getCurrentBlock();
    if (cur) cur->hasTerminator = true;
}

void CodeGen::visit(ExprStmt &node) {
    if (node.getExpr()) {
        node.getExpr()->accept(*this);
    }
}

void CodeGen::visit(NumberExpr &node) {
    if (node.getIsFloat()) {
        lastValue = {std::to_string(node.getFloatVal()), {TypeKind::Float}, false};
    } else {
        lastValue = {std::to_string(node.getIntVal()), {TypeKind::Int}, false};
    }
}

void CodeGen::visit(BoolExpr &node) {
    lastValue = {node.getValue() ? "1" : "0", {TypeKind::Bool}, false};
}

void CodeGen::visit(StringLiteralExpr &node) {
    std::string constName = "@.str." + std::to_string(++stringLiteralCount);
    stringConstants.push_back({constName, node.getValue()});

    std::string gepReg = newRegister();
    int len = (int)node.getValue().size() + 1;
    emitInstruction(gepReg + " = getelementptr inbounds [" + std::to_string(len) + " x i8], [" +
                    std::to_string(len) + " x i8]* " + constName + ", i64 0, i64 0");
    lastValue = {gepReg, {TypeKind::String}, false};
}

void CodeGen::visit(VariableExpr &node) {
    LLVMValue varSlot = getVariable(node.getName());
    std::string loadReg = newRegister();
    std::string ty = llvmType(varSlot.type);
    emitInstruction(loadReg + " = load " + ty + ", " + ty + "* " + varSlot.name);
    lastValue = {loadReg, varSlot.type, false};
}

void CodeGen::visit(ArrayAccessExpr &node) {
    LLVMValue varSlot = getVariable(node.getName());
    node.getIndex()->accept(*this);
    LLVMValue idxVal = lastValue;

    std::string elemTy = (varSlot.type.elementType == TypeKind::Float) ? "double" : "i32";
    std::string gepReg = newRegister();

    if (varSlot.type.arraySize > 0) {
        std::string arrTy = "[" + std::to_string(varSlot.type.arraySize) + " x " + elemTy + "]";
        emitInstruction(gepReg + " = getelementptr inbounds " + arrTy + ", " + arrTy + "* " +
                        varSlot.name + ", i64 0, i32 " + idxVal.name);
    } else {
        emitInstruction(gepReg + " = getelementptr inbounds " + elemTy + ", " + elemTy + "* " +
                        varSlot.name + ", i32 " + idxVal.name);
    }

    std::string loadReg = newRegister();
    emitInstruction(loadReg + " = load " + elemTy + ", " + elemTy + "* " + gepReg);
    lastValue = {loadReg, {varSlot.type.elementType}, false};
}

void CodeGen::visit(UnaryExpr &node) {
    node.getOperand()->accept(*this);
    LLVMValue opVal = lastValue;
    std::string resReg = newRegister();

    if (node.getOp() == '-') {
        if (opVal.type.kind == TypeKind::Float) {
            emitInstruction(resReg + " = fneg double " + opVal.name);
        } else {
            emitInstruction(resReg + " = sub i32 0, " + opVal.name);
        }
        lastValue = {resReg, opVal.type, false};
    } else if (node.getOp() == '!') {
        emitInstruction(resReg + " = xor i1 " + opVal.name + ", 1");
        lastValue = {resReg, {TypeKind::Bool}, false};
    }
}

void CodeGen::visit(BinaryExpr &node) {
    node.getLHS()->accept(*this);
    LLVMValue lhsVal = lastValue;

    node.getRHS()->accept(*this);
    LLVMValue rhsVal = lastValue;

    std::string resReg = newRegister();
    bool isFloat = (lhsVal.type.kind == TypeKind::Float || rhsVal.type.kind == TypeKind::Float);

    int op = node.getOp();
    switch (op) {
        case '+':
            if (isFloat) emitInstruction(resReg + " = fadd double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = add i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, isFloat ? TypeInfo{TypeKind::Float} : TypeInfo{TypeKind::Int}, false};
            break;
        case '-':
            if (isFloat) emitInstruction(resReg + " = fsub double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = sub i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, isFloat ? TypeInfo{TypeKind::Float} : TypeInfo{TypeKind::Int}, false};
            break;
        case '*':
            if (isFloat) emitInstruction(resReg + " = fmul double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = mul i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, isFloat ? TypeInfo{TypeKind::Float} : TypeInfo{TypeKind::Int}, false};
            break;
        case '/':
            if (isFloat) emitInstruction(resReg + " = fdiv double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = sdiv i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, isFloat ? TypeInfo{TypeKind::Float} : TypeInfo{TypeKind::Int}, false};
            break;
        case '%':
            emitInstruction(resReg + " = srem i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Int}, false};
            break;
        case '<':
            if (isFloat) emitInstruction(resReg + " = fcmp olt double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = icmp slt i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Bool}, false};
            break;
        case '>':
            if (isFloat) emitInstruction(resReg + " = fcmp ogt double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = icmp sgt i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Bool}, false};
            break;
        case tok_le:
            if (isFloat) emitInstruction(resReg + " = fcmp ole double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = icmp sle i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Bool}, false};
            break;
        case tok_ge:
            if (isFloat) emitInstruction(resReg + " = fcmp oge double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = icmp sge i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Bool}, false};
            break;
        case tok_eq:
            if (isFloat) emitInstruction(resReg + " = fcmp oeq double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = icmp eq i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Bool}, false};
            break;
        case tok_ne:
            if (isFloat) emitInstruction(resReg + " = fcmp one double " + lhsVal.name + ", " + rhsVal.name);
            else emitInstruction(resReg + " = icmp ne i32 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Bool}, false};
            break;
        case tok_and:
            emitInstruction(resReg + " = and i1 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Bool}, false};
            break;
        case tok_or:
            emitInstruction(resReg + " = or i1 " + lhsVal.name + ", " + rhsVal.name);
            lastValue = {resReg, {TypeKind::Bool}, false};
            break;
        default:
            break;
    }
}

void CodeGen::visit(AssignExpr &node) {
    node.getValue()->accept(*this);
    LLVMValue rhsVal = lastValue;

    LLVMValue varSlot = getVariable(node.getVarName());
    if (node.getIsArrayAssign()) {
        node.getArrayIndex()->accept(*this);
        LLVMValue idxVal = lastValue;

        std::string elemTy = (varSlot.type.elementType == TypeKind::Float) ? "double" : "i32";
        std::string gepReg = newRegister();

        if (varSlot.type.arraySize > 0) {
            std::string arrTy = "[" + std::to_string(varSlot.type.arraySize) + " x " + elemTy + "]";
            emitInstruction(gepReg + " = getelementptr inbounds " + arrTy + ", " + arrTy + "* " +
                            varSlot.name + ", i64 0, i32 " + idxVal.name);
        } else {
            emitInstruction(gepReg + " = getelementptr inbounds " + elemTy + ", " + elemTy + "* " +
                            varSlot.name + ", i32 " + idxVal.name);
        }
        emitInstruction("store " + elemTy + " " + rhsVal.name + ", " + elemTy + "* " + gepReg);
    } else {
        std::string ty = llvmType(varSlot.type);
        emitInstruction("store " + ty + " " + rhsVal.name + ", " + ty + "* " + varSlot.name);
    }
    lastValue = rhsVal;
}

void CodeGen::visit(CallExpr &node) {
    std::vector<LLVMValue> argVals;
    for (const auto &arg : node.getArgs()) {
        arg->accept(*this);
        argVals.push_back(lastValue);
    }

    std::string retTy = llvmType(node.getType());
    std::string callStr;
    std::string resReg;

    if (node.getType().kind != TypeKind::Void) {
        resReg = newRegister();
        callStr = resReg + " = call " + retTy + " @" + node.getCallee() + "(";
    } else {
        callStr = "call void @" + node.getCallee() + "(";
    }

    for (size_t i = 0; i < argVals.size(); ++i) {
        callStr += llvmType(argVals[i].type) + " " + argVals[i].name;
        if (i + 1 < argVals.size()) callStr += ", ";
    }
    callStr += ")";

    emitInstruction(callStr);
    if (!resReg.empty()) {
        lastValue = {resReg, node.getType(), false};
    } else {
        lastValue = {"", {TypeKind::Void}, false};
    }
}

} // namespace algoc

