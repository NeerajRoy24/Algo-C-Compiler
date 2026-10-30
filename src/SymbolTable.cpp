#include "SymbolTable.h"
#include <iomanip>

namespace algoc {

SymbolTable::SymbolTable() {
    // Global scope at level 0
    currentScope = std::make_shared<Scope>(0, nullptr);
    allScopes.push_back(currentScope);
}

void SymbolTable::enterScope() {
    currentLevel++;
    currentScope = std::make_shared<Scope>(currentLevel, currentScope);
    allScopes.push_back(currentScope);
}

void SymbolTable::exitScope() {
    if (currentScope && currentScope->getParent()) {
        currentScope = currentScope->getParent();
        currentLevel--;
    }
}

bool SymbolTable::insert(const Symbol &sym) {
    if (!currentScope) return false;
    Symbol s = sym;
    s.scopeLevel = currentLevel;
    return currentScope->insert(s);
}

const Symbol *SymbolTable::lookup(const std::string &name) const {
    if (!currentScope) return nullptr;
    return currentScope->lookup(name);
}

const Symbol *SymbolTable::lookupLocal(const std::string &name) const {
    if (!currentScope) return nullptr;
    return currentScope->lookupLocal(name);
}

void SymbolTable::dump() const {
    std::cout << "\n============================== SYMBOL TABLE DUMP ==============================\n";
    std::cout << std::left << std::setw(18) << "Name"
              << std::setw(14) << "Kind"
              << std::setw(16) << "Type"
              << std::setw(10) << "Scope"
              << std::setw(12) << "Location"
              << "\n";
    std::cout << "-------------------------------------------------------------------------------\n";

    for (const auto &sc : allScopes) {
        for (const auto &entry : sc->getSymbols()) {
            const Symbol &sym = entry.second;
            std::string locStr = std::to_string(sym.location.line) + ":" + std::to_string(sym.location.column);
            std::cout << std::left << std::setw(18) << sym.name
                      << std::setw(14) << sym.kindToString()
                      << std::setw(16) << sym.type.toString()
                      << std::setw(10) << sym.scopeLevel
                      << std::setw(12) << locStr
                      << "\n";
        }
    }
    std::cout << "===============================================================================\n\n";
}

} // namespace algoc

