# ⚡ Algo-C Compiler & Live Web Studio

[![Live Demo](https://img.shields.io/badge/Live%20Demo-GitHub%20Pages-38bdf8?style=for-the-badge&logo=googlechrome&logoColor=white)](https://neerajroy24.github.io/Algo-C-Compiler/)
[![Language](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![LLVM](https://img.shields.io/badge/LLVM-18%2B%20IR-9333ea?style=for-the-badge&logo=llvm&logoColor=white)](https://llvm.org/)
[![Target](https://img.shields.io/badge/Target-x86--64%20Gas-10b981?style=for-the-badge)](https://en.wikipedia.org/wiki/X86-64)
[![Course](https://img.shields.io/badge/Course-VL2026270103371-f59e0b?style=for-the-badge)](https://github.com/NeerajRoy24/Algo-C-Compiler)

> **Live Interactive Studio**: [https://neerajroy24.github.io/Algo-C-Compiler/](https://neerajroy24.github.io/Algo-C-Compiler/)  
> **Course**: VL2026270103371 (Compiler Design Laboratory)  
> **Author**: Neeraj Roy (24BDS0427)

---

## 📖 Project Overview

**Algo-C** is a high-performance algorithmic programming language and compiler implementing an end-to-end 7-stage compilation pipeline. Developed as part of the Compiler Design Laboratory curriculum, it features a complete lexical analyzer, recursive descent parser with operator precedence climbing, typed LLVM-style RTTI Abstract Syntax Tree, scoped semantic analysis, multi-pass optimization engine (-O1), authentic LLVM IR generation, x86-64 target assembly emission, and a stack-based virtual machine interpreter.

The project includes an **interactive in-browser web studio** hosted on GitHub Pages that allows anyone to write, compile, inspect, and execute Algo-C programs directly without installing any local compiler dependencies.

---

## 🌐 Live Web Demo & Compiler Studio

Visit the live demo directly in your browser:
👉 **[https://neerajroy24.github.io/Algo-C-Compiler/](https://neerajroy24.github.io/Algo-C-Compiler/)**

### Web Studio Features:
- **Interactive Code Editor**: Write custom Algo-C programs or load presets with 1 click.
- **In-Browser Execution Console**: Runs program logic with simulated standard I/O (`print_str`, `print_int`, `println`), cycle counting, and exit codes.
- **Authentic LLVM IR Viewer**: Syntax-highlighted LLVM 18+ Intermediate Representation matching native output.
- **x86-64 Target Assembly**: AT&T assembly code with register annotations and calling convention frames.
- **Optimization Inspector**: Real-time metrics for constant folding, algebraic simplifications, and dead branch pruning.
- **Scoped Symbol Table**: Inspect nested lexical scopes, variable identifiers, function signatures, and memory layouts.
- **AST Node Visualizer**: Collapsible visual tree representation of modules, statements, expressions, and literals.
- **Lexical Token Stream**: Complete token breakdown with kinds, lexemes, and source locations.
- **Semantic Diagnostics**: Diagnostic callout cards highlighting compile-time type errors, undeclared variables, duplicate identifiers, and missing returns.

---

## 🏛️ Compiler Pipeline Architecture

```
                                  ALGO-C COMPILATION PIPELINE
                                  
     [ Source Code (.algoc) ]
                │
                ▼
     ┌──────────────────────┐
     │ 1. Lexical Analysis  │  ──> Regular definitions, finite automata, token stream
     └──────────────────────┘
                │
                ▼
     ┌──────────────────────┐
     │ 2. Syntax Analysis   │  ──> Recursive descent parser with operator precedence climbing
     └──────────────────────┘
                │
                ▼
     ┌──────────────────────┐
     │ 3. AST Construction │  ──> LLVM-style RTTI typed AST hierarchy with node visualizer
     └──────────────────────┘
                │
                ▼
     ┌──────────────────────┐
     │ 4. Semantic Analysis │  ──> Multi-level scoped symbol table, strict type checks,
     └──────────────────────┘      array bound validation & definite-return path analysis
                │
                ▼
     ┌──────────────────────┐
     │ 5. Code Optimization │  ──> Constant folding, algebraic identities (x+0=x, x*1=x, x*0=0),
     └──────────────────────┘      dead branch pruning & Dead Code Elimination (-O1)
                │
                ▼
     ┌──────────────────────┐
     │ 6. Code Generation   │  ──> Authentic LLVM IR (.ll) & AMD64 / x86-64 target assembly (.s)
     └──────────────────────┘
                │
                ▼
     ┌──────────────────────┐
     │ 7. Virtual Machine   │  ──> Bytecode stack VM execution with standard I/O library
     └──────────────────────┘
```

---

## 📋 Course Review Milestones & Deliverables

### Review 1: Front-End Core (Lexer, Parser & AST)
- [x] Context-Free Grammar (CFG) specification for algorithmic design syntax.
- [x] Multi-character token handling (`==`, `!=`, `<=`, `>=`, `&&`, `||`).
- [x] Source location tracking (line numbers and column numbers).
- [x] Recursive-descent parser with operator precedence climbing.
- [x] Abstract Syntax Tree with LLVM-style Runtime Type Information (RTTI) and `dump()` method.
- [x] Command-line flags: `--dump-tokens` and `--dump-ast`.

### Review 2: Middle-End Analysis (Semantic Checks & Symbol Table)
- [x] Multi-level scoped Symbol Table supporting function, variable, and fixed-size array declarations.
- [x] Strict compile-time type checking for assignments, conditions, arithmetic, and returns.
- [x] Diagnostic error reporting with line/column markers for undeclared identifiers and redefinitions.
- [x] Definite-return path analysis verifying that non-void functions always return a value.
- [x] Command-line flags: `--dump-symtab` and `--check-only`.

### Review 3: Back-End Synthesis (Optimizer, LLVM IR, Assembly & VM)
- [x] Multi-pass optimization engine (`-O1`, `--optimize`):
  - Constant folding on numeric and boolean literals.
  - Algebraic simplification: `x + 0 -> x`, `x - 0 -> x`, `x * 1 -> x`, `x * 0 -> 0`.
  - Dead branch pruning: `if (false) { ... } else { ... }`.
- [x] LLVM IR Generator producing valid LLVM 18+ Intermediate Representation (`--emit-llvm`).
- [x] Target Code Generator emitting x86-64 AT&T gas assembly (`--emit-asm`).
- [x] Bytecode Virtual Machine interpreter with standard library builtins (`--run`).

---

## 🧪 Benchmark Test Cases

The compiler includes 6 standard benchmark suites located in the `tests/` directory:

| Test File | Description | Features Tested |
|---|---|---|
| `test1_gcd.algoc` | Euclidean GCD algorithm | While loops, arithmetic, modulo, return values |
| `test2_while_counter.algoc` | While loop counter | Basic blocks, control flow, function calls |
| `test3_fibonacci.algoc` | Recursive Fibonacci sequence | Recursion, conditionals, stack frames |
| `test4_bubble_sort.algoc` | Bubble Sort on fixed array | Array declaration, indexing, nested loops |
| `test5_semantic_errors.algoc` | Semantic analyzer test | Type mismatch, undeclared identifier, duplicate name, missing return |
| `test6_optimization.algoc` | Optimizer verification | Constant folding, algebraic reduction, dead branch pruning |

---

## 🛠️ Building & Running Locally

### Prerequisites
- GCC / MinGW-w64 (C++17 support) or Clang or MSVC
- CMake (optional, version 3.14+)

### Quick Build (Windows Batch Script)
```cmd
.\build.bat
```

### Manual GCC Build
```bash
g++ -std=c++17 -Wall -Iinclude -O2 -o algoc.exe \
    src/main.cpp src/Lexer.cpp src/AST.cpp src/Parser.cpp \
    src/SymbolTable.cpp src/SemanticAnalyzer.cpp src/CodeGen.cpp \
    src/Optimizer.cpp src/TargetCodeGen.cpp src/VM.cpp
```

### CMake Build
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## 💻 CLI Usage Guide

```text
Usage: algoc.exe [options] <source-file.algoc>

Options:
  --dump-tokens     Tokenize source and print token stream (Review 1)
  --dump-ast        Parse source and print Abstract Syntax Tree (Review 1)
  --dump-symtab     Run semantic analysis and print scoped Symbol Table (Review 2)
  --check-only      Run semantic type checks & definite-return analysis (Review 2)
  --emit-llvm       Generate authentic LLVM IR (.ll) file (Review 2 / Review 3)
  --emit-asm        Generate x86-64 target assembly (.s) file (Review 3)
  --optimize, -O1   Run optimization passes (constant folding, DCE, algebraic) (Review 3)
  --run             Execute the compiled algorithm directly via VM (Review 3)
  -o <filename>     Specify output file path
  --help, -h        Display this help message
```

### Example Commands:
```bash
# Execute GCD algorithm directly:
.\algoc.exe --run tests\test1_gcd.algoc

# Emit LLVM IR to a file:
.\algoc.exe --emit-llvm -o gcd.ll tests\test1_gcd.algoc

# Run the optimization pass:
.\algoc.exe --optimize tests\test6_optimization.algoc

# Verify semantic diagnostics:
.\algoc.exe --check-only tests\test5_semantic_errors.algoc
```

---

## 📂 Project Structure

```
├── include/
│   ├── AST.h               # Abstract Syntax Tree node hierarchy & RTTI
│   ├── CodeGen.h           # LLVM IR code generator interface
│   ├── Lexer.h             # Tokenizer & source location tracking
│   ├── Optimizer.h         # Constant folding & dead code elimination
│   ├── Parser.h            # Recursive descent parser with precedence climbing
│   ├── SemanticAnalyzer.h  # Scoped type checker & return analyzer
│   ├── SymbolTable.h       # Lexical scope & symbol definitions
│   ├── TargetCodeGen.h     # x86-64 target assembly generator
│   └── VM.h                # Bytecode compiler & stack virtual machine
├── src/
│   ├── AST.cpp
│   ├── CodeGen.cpp
│   ├── Lexer.cpp
│   ├── main.cpp            # Compiler CLI driver entrypoint
│   ├── Optimizer.cpp
│   ├── Parser.cpp
│   ├── SemanticAnalyzer.cpp
│   ├── SymbolTable.cpp
│   ├── TargetCodeGen.cpp
│   └── VM.cpp
├── tests/                  # Standard benchmark programs (.algoc)
│   ├── test1_gcd.algoc
│   ├── test2_while_counter.algoc
│   ├── test3_fibonacci.algoc
│   ├── test4_bubble_sort.algoc
│   ├── test5_semantic_errors.algoc
│   └── test6_optimization.algoc
├── index.html              # Interactive Web Studio & Live Demo
├── style.css               # Studio styling, glassmorphism & dark UI
├── compiler.js             # 100% in-browser compiler engine
├── build.bat               # Windows build script
├── CMakeLists.txt          # CMake cross-platform configuration
├── Makefile                # Unix / Make configuration
└── README.md               # Documentation & project manual
```

---

## 📜 License & Acknowledgments

This project is submitted for course **VL2026270103371 (Compiler Design Laboratory)** by **Neeraj Roy (Registration No: 24BDS0427)**.  
Licensed under the [MIT License](LICENSE).
