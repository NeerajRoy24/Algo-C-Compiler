cmake_minimum_required(VERSION 3.14)
project(AlgoC VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/AST.cpp
    src/Parser.cpp
    src/SymbolTable.cpp
    src/SemanticAnalyzer.cpp
    src/CodeGen.cpp
    src/Optimizer.cpp
    src/TargetCodeGen.cpp
    src/VM.cpp
)

add_executable(algoc ${SOURCES})

