#ifndef ALGO_C_SYMBOL_TABLE_H
#define ALGO_C_SYMBOL_TABLE_H

#include "AST.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

namespace algoc {

enum class SymbolKind {
    Variable,
    Parameter,
    Function,
    Array
};

struct Symbol {
    std::string name;
    TypeInfo type;
    SymbolKind kind;
    int scopeLevel = 0;
    SourceLocation location;
    std::vector<TypeInfo> paramTypes; // if Function

    std::string kindToString() const {
        switch (kind) {
            case SymbolKind::Variable: return "Variable";
            case SymbolKind::Parameter: return "Parameter";
            case SymbolKind::Function: return "Function";
            case SymbolKind::Array: return "Array";
        }
        return "Unknown";
    }
};

class Scope {
public:
    explicit Scope(int level, std::shared_ptr<Scope> parent = nullptr)
        : level(level), parent(parent) {}

    bool insert(const Symbol &symbol) {
        if (symbols.find(symbol.name) != symbols.end()) {
            return false; // Redeclaration in same scope
        }
        symbols[symbol.name] = symbol;
        return true;
    }

    const Symbol *lookup(const std::string &name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &it->second;
        }
        if (parent) {
            return parent->lookup(name);
        }
        return nullptr;
    }

    const Symbol *lookupLocal(const std::string &name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &it->second;
        }
        return nullptr;
    }

    int getLevel() const { return level; }
    std::shared_ptr<Scope> getParent() const { return parent; }
    const std::unordered_map<std::string, Symbol> &getSymbols() const { return symbols; }

private:
    int level;
    std::shared_ptr<Scope> parent;
    std::unordered_map<std::string, Symbol> symbols;
};

class SymbolTable {
public:
    SymbolTable();

    void enterScope();
    void exitScope();

    bool insert(const Symbol &sym);
    const Symbol *lookup(const std::string &name) const;
    const Symbol *lookupLocal(const std::string &name) const;

    int getCurrentScopeLevel() const { return currentScope ? currentScope->getLevel() : 0; }
    void dump() const;

private:
    std::shared_ptr<Scope> currentScope;
    std::vector<std::shared_ptr<Scope>> allScopes; // keeps track of all created scopes for inspection
    int currentLevel = 0;
};

} // namespace algoc

#endif // ALGO_C_SYMBOL_TABLE_H

