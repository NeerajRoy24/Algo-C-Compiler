#ifndef ALGO_C_TARGET_CODEGEN_H
#define ALGO_C_TARGET_CODEGEN_H

#include "AST.h"
#include <string>
#include <sstream>

namespace algoc {

class TargetCodeGen {
public:
    TargetCodeGen() = default;

    std::string generateAssembly(ModuleAST &module);
};

} // namespace algoc

#endif // ALGO_C_TARGET_CODEGEN_H

