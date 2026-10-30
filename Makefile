CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -O2
SRCS = src/main.cpp src/Lexer.cpp src/AST.cpp src/Parser.cpp src/SymbolTable.cpp src/SemanticAnalyzer.cpp src/CodeGen.cpp src/Optimizer.cpp src/TargetCodeGen.cpp src/VM.cpp
TARGET = algoc.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.ll *.s *.o

