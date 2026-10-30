#ifndef ALGO_C_OPTIMIZER_H
#define ALGO_C_OPTIMIZER_H

#include "AST.h"
#include <string>
#include <memory>

namespace algoc {

struct OptimizationStats {
    int constantFolds = 0;
    int algebraicSimplifications = 0;
    int deadBranchesEliminated = 0;
    int deadStatementsEliminated = 0;

    void dump() const;
};

class Optimizer {
public:
    Optimizer() = default;

    OptimizationStats optimize(ModuleAST &module);

private:
    OptimizationStats stats;

    std::unique_ptr<ExprAST> optimizeExpr(std::unique_ptr<ExprAST> expr);
    std::unique_ptr<StmtAST> optimizeStmt(std::unique_ptr<StmtAST> stmt);
    void optimizeBlock(BlockStmt &block);
};

} // namespace algoc

#endif // ALGO_C_OPTIMIZER_H
