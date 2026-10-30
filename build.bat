@echo off
echo =======================================================
echo Building Algo-C Compiler (C++17)
echo =======================================================

set GXX=C:\msys64\ucrt64\bin\g++.exe

if exist "%GXX%" (
    "%GXX%" -std=c++17 -Wall -Iinclude -O2 -o algoc.exe src/main.cpp src/Lexer.cpp src/AST.cpp src/Parser.cpp src/SymbolTable.cpp src/SemanticAnalyzer.cpp src/CodeGen.cpp src/Optimizer.cpp src/TargetCodeGen.cpp src/VM.cpp
) else (
    g++ -std=c++17 -Wall -Iinclude -O2 -o algoc.exe src/main.cpp src/Lexer.cpp src/AST.cpp src/Parser.cpp src/SymbolTable.cpp src/SemanticAnalyzer.cpp src/CodeGen.cpp src/Optimizer.cpp src/TargetCodeGen.cpp src/VM.cpp
)

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] algoc.exe compiled successfully!
) else (
    echo [ERROR] Compilation failed.
)

