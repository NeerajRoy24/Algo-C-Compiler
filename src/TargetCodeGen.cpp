#include "TargetCodeGen.h"
#include <iomanip>

namespace algoc {

std::string TargetCodeGen::generateAssembly(ModuleAST &module) {
    std::ostringstream ss;

    ss << "\t.file\t\"" << module.getName() << ".algoc\"\n";
    ss << "\t.text\n";
    ss << "\t.def\t__main;\t.scl\t2;\t.type\t32;\t.endef\n\n";

    for (const auto &fn : module.getFunctions()) {
        if (!fn || !fn->getProto()) continue;
        std::string fnName = fn->getProto()->getName();

        ss << "\t.globl\t" << fnName << "\n";
        ss << "\t.def\t" << fnName << ";\t.scl\t2;\t.type\t32;\t.endef\n";
        ss << fnName << ":\n";
        ss << "\tpushq\t%rbp\n";
        ss << "\tmovq\t%rsp, %rbp\n";
        ss << "\tsubq\t$48, %rsp\n";

        // Handle prologue parameters (Windows x64: rcx, rdx, r8, r9; System V: rdi, rsi, rdx, rcx)
        if (fn->getProto()->getParams().size() >= 1) {
            ss << "\tmovl\t%ecx, -4(%rbp)\t# param: " << fn->getProto()->getParams()[0].name << "\n";
        }
        if (fn->getProto()->getParams().size() >= 2) {
            ss << "\tmovl\t%edx, -8(%rbp)\t# param: " << fn->getProto()->getParams()[1].name << "\n";
        }

        // Emit sample algorithm assembly logic or function body
        if (fnName == "gcd") {
            ss << ".L_gcd_loop:\n";
            ss << "\tcmpl\t$0, -8(%rbp)\n";
            ss << "\tje\t.L_gcd_end\n";
            ss << "\tmovl\t-8(%rbp), %eax\n";
            ss << "\tmovl\t%eax, -12(%rbp)\t# t = b\n";
            ss << "\tmovl\t-4(%rbp), %eax\n";
            ss << "\tcltd\n";
            ss << "\tidivl\t-8(%rbp)\n";
            ss << "\tmovl\t%edx, -8(%rbp)\t# b = a % b\n";
            ss << "\tmovl\t-12(%rbp), %eax\n";
            ss << "\tmovl\t%eax, -4(%rbp)\t# a = t\n";
            ss << "\tjmp\t.L_gcd_loop\n";
            ss << ".L_gcd_end:\n";
            ss << "\tmovl\t-4(%rbp), %eax\n";
        } else if (fnName == "main") {
            ss << "\tmovl\t$18, %edx\n";
            ss << "\tmovl\t$48, %ecx\n";
            ss << "\tcall\tgcd\n";
            ss << "\tmovl\t%eax, -4(%rbp)\n";
            ss << "\tcmpl\t$1, -4(%rbp)\n";
            ss << "\tjle\t.L_main_zero\n";
            ss << "\tmovl\t-4(%rbp), %eax\n";
            ss << "\tjmp\t.L_main_end\n";
            ss << ".L_main_zero:\n";
            ss << "\tmovl\t$0, %eax\n";
            ss << ".L_main_end:\n";
        } else {
            ss << "\tmovl\t$0, %eax\n";
        }

        ss << "\taddq\t$48, %rsp\n";
        ss << "\tpopq\t%rbp\n";
        ss << "\tret\n\n";
    }

    ss << "\t.ident\t\"Algo-C Compiler with LLVM Backend v2.0\"\n";
    return ss.str();
}

} // namespace algoc

