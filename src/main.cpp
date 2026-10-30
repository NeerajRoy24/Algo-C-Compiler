#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "CodeGen.h"
#include "Optimizer.h"
#include "TargetCodeGen.h"
#include "VM.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

using namespace algoc;

void printUsage(const char *progName) {
    std::cout << "Algo-C Compiler (LLVM Backend & Algorithmic Design Language)\n";
    std::cout << "Author: 24BDS0427 | Course: VL2026270103371 (Compiler Design Lab)\n\n";
    std::cout << "Usage: " << progName << " [options] <source-file.algoc>\n\n";
    std::cout << "Options:\n";
    std::cout << "  --dump-tokens     Tokenize source and print token stream (Review 1)\n";
    std::cout << "  --dump-ast        Parse source and print Abstract Syntax Tree (Review 1)\n";
    std::cout << "  --dump-symtab     Run semantic analysis and print scoped Symbol Table (Review 2)\n";
    std::cout << "  --check-only      Run semantic type checks & definite-return analysis (Review 2)\n";
    std::cout << "  --emit-llvm       Generate authentic LLVM IR (.ll) file (Review 2 / Review 3)\n";
    std::cout << "  --emit-asm        Generate x86-64 target assembly (.s) file (Review 3)\n";
    std::cout << "  --optimize, -O1   Run optimization passes (constant folding, DCE, algebraic) (Review 3)\n";
    std::cout << "  --run             Execute the compiled algorithm directly and display output (Review 3)\n";
    std::cout << "  -o <filename>     Specify output file path\n";
    std::cout << "  --help, -h        Display this help message\n";
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile;
    std::string outputFile;
    bool dumpTokens = false;
    bool dumpAST = false;
    bool dumpSymTab = false;
    bool checkOnly = false;
    bool emitLLVM = false;
    bool emitAsm = false;
    bool doOptimize = false;
    bool doRun = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--dump-tokens") {
            dumpTokens = true;
        } else if (arg == "--dump-ast") {
            dumpAST = true;
        } else if (arg == "--dump-symtab") {
            dumpSymTab = true;
        } else if (arg == "--check-only") {
            checkOnly = true;
        } else if (arg == "--emit-llvm") {
            emitLLVM = true;
        } else if (arg == "--emit-asm") {
            emitAsm = true;
        } else if (arg == "--optimize" || arg == "-O1" || arg == "-O") {
            doOptimize = true;
        } else if (arg == "--run") {
            doRun = true;
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg[0] != '-') {
            inputFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified.\n";
        return 1;
    }

    std::ifstream file(inputFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << inputFile << "'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    // 1. Phase 1: Lexical Analysis
    if (dumpTokens) {
        std::cout << "=== Lexical Analysis: Token Stream for " << inputFile << " ===\n";
        Lexer lex(source, inputFile);
        Token tok;
        do {
            tok = lex.getNextToken();
            std::cout << tok.toString() << "\n";
        } while (tok.kind != tok_eof);
        return 0;
    }

    // 2. Phase 2: Syntax Analysis (Parser & AST)
    Lexer lexer(source, inputFile);
    Parser parser(lexer);
    auto module = parser.parseModule();

    if (parser.hasErrors()) {
        std::cerr << "\n[!] Compilation failed during Parsing:\n";
        for (const auto &err : parser.getErrors()) {
            std::cerr << "  " << err << "\n";
        }
        return 1;
    }

    if (dumpAST) {
        std::cout << "\n=== Abstract Syntax Tree (LLVM-Style RTTI Nodes) ===\n";
        module->dump();
        return 0;
    }

    // 3. Phase 3: Semantic Analysis & Symbol Table (Review 2)
    SemanticAnalyzer sema;
    bool semaSuccess = sema.analyze(*module);

    if (dumpSymTab) {
        sema.getSymbolTable().dump();
    }

    if (!semaSuccess) {
        std::cerr << "\n[!] Semantic Analysis failed with " << sema.getErrors().size() << " error(s):\n";
        for (const auto &err : sema.getErrors()) {
            std::cerr << "  " << err << "\n";
        }
        return 1;
    }

    if (checkOnly) {
        std::cout << "[+] Semantic Analysis Successful! No type or scope errors found.\n";
        return 0;
    }

    // 4. Phase 4: Code Optimization (Review 3)
    if (doOptimize) {
        Optimizer opt;
        auto stats = opt.optimize(*module);
        stats.dump();
    }

    // 5. Phase 5: Intermediate Code Generation (LLVM IR)
    CodeGen codegen;
    std::string llvmIR = codegen.generateIR(*module);

    if (emitLLVM || (!emitAsm && !doRun && outputFile.empty())) {
        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            out << llvmIR;
            out.close();
            std::cout << "[+] LLVM IR emitted to " << outputFile << "\n";
        } else if (emitLLVM) {
            std::cout << "\n=== Generated LLVM IR ===\n";
            std::cout << llvmIR;
        }
    }

    // 6. Phase 6: Target Code Emission (.s Assembly)
    if (emitAsm) {
        TargetCodeGen targetGen;
        std::string asmCode = targetGen.generateAssembly(*module);
        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            out << asmCode;
            out.close();
            std::cout << "[+] Assembly emitted to " << outputFile << "\n";
        } else {
            std::cout << "\n=== Generated x86-64 Target Assembly ===\n";
            std::cout << asmCode;
        }
    }

    // 7. Phase 7: Interactive Execution via VM
    if (doRun) {
        std::cout << "=== Executing Algo-C Program: " << inputFile << " ===\n";
        VMCompiler vmComp;
        auto compiledFns = vmComp.compile(*module);
        VirtualMachine vm;
        int exitCode = vm.run(compiledFns, "main");
        std::cout << "\n[Program finished with exit code: " << exitCode << "]\n";
    }

    return 0;
}

