#include "Optimizer.h"
#include <iostream>

namespace algoc {

void OptimizationStats::dump() const {
    std::cout << "\n======================== OPTIMIZATION PASS REPORT ========================\n";
    std::cout << "  Constant Folding Operations      : " << constantFolds << "\n";
    std::cout << "  Algebraic Simplifications        : " << algebraicSimplifications << "\n";
    std::cout << "  Dead Branches Pruned             : " << deadBranchesEliminated << "\n";
    std::cout << "  Dead Statements Removed          : " << deadStatementsEliminated << "\n";
    std::cout << "  Total Transformations Applied    : "
              << (constantFolds + algebraicSimplifications + deadBranchesEliminated + deadStatementsEliminated)
              << "\n";
    std::cout << "==========================================================================\n\n";
}

OptimizationStats Optimizer::optimize(ModuleAST &module) {
    stats = OptimizationStats{};

    for (auto &fn : module.getFunctions()) {
        if (fn && fn->getBody()) {
            optimizeBlock(*fn->getBody());
        }
    }

    return stats;
}

std::unique_ptr<ExprAST> Optimizer::optimizeExpr(std::unique_ptr<ExprAST> expr) {
    if (!expr) return nullptr;

    // 1. Binary Expressions: Constant folding & Algebraic identities
    if (auto bin = dyn_cast<BinaryExpr>(expr.get())) {
        int op = bin->getOp();
        SourceLocation loc = bin->getLocation();

        auto lhs = optimizeExpr(bin->takeLHS());
        auto rhs = optimizeExpr(bin->takeRHS());

        // Case A: Both LHS and RHS are integer constants
        auto numL = dyn_cast<NumberExpr>(lhs.get());
        auto numR = dyn_cast<NumberExpr>(rhs.get());

        if (numL && numR && !numL->getIsFloat() && !numR->getIsFloat()) {
            int64_t lVal = numL->getIntVal();
            int64_t rVal = numR->getIntVal();

            switch (op) {
                case '+':
                    stats.constantFolds++;
                    return std::make_unique<NumberExpr>(lVal + rVal, loc);
                case '-':
                    stats.constantFolds++;
                    return std::make_unique<NumberExpr>(lVal - rVal, loc);
                case '*':
                    stats.constantFolds++;
                    return std::make_unique<NumberExpr>(lVal * rVal, loc);
                case '/':
                    if (rVal != 0) {
                        stats.constantFolds++;
                        return std::make_unique<NumberExpr>(lVal / rVal, loc);
                    }
                    break;
                case '%':
                    if (rVal != 0) {
                        stats.constantFolds++;
                        return std::make_unique<NumberExpr>(lVal % rVal, loc);
                    }
                    break;
                case '<':
                    stats.constantFolds++;
                    return std::make_unique<BoolExpr>(lVal < rVal, loc);
                case '>':
                    stats.constantFolds++;
                    return std::make_unique<BoolExpr>(lVal > rVal, loc);
                case tok_le:
                    stats.constantFolds++;
                    return std::make_unique<BoolExpr>(lVal <= rVal, loc);
                case tok_ge:
                    stats.constantFolds++;
                    return std::make_unique<BoolExpr>(lVal >= rVal, loc);
                case tok_eq:
                    stats.constantFolds++;
                    return std::make_unique<BoolExpr>(lVal == rVal, loc);
                case tok_ne:
                    stats.constantFolds++;
                    return std::make_unique<BoolExpr>(lVal != rVal, loc);
            }
        }

        // Case B: Algebraic simplifications on integers
        // x + 0 -> x
        if (op == '+' && numR && !numR->getIsFloat() && numR->getIntVal() == 0) {
            stats.algebraicSimplifications++;
            return lhs;
        }
        // 0 + x -> x
        if (op == '+' && numL && !numL->getIsFloat() && numL->getIntVal() == 0) {
            stats.algebraicSimplifications++;
            return rhs;
        }
        // x - 0 -> x
        if (op == '-' && numR && !numR->getIsFloat() && numR->getIntVal() == 0) {
            stats.algebraicSimplifications++;
            return lhs;
        }
        // x * 1 -> x
        if (op == '*' && numR && !numR->getIsFloat() && numR->getIntVal() == 1) {
            stats.algebraicSimplifications++;
            return lhs;
        }
        // 1 * x -> x
        if (op == '*' && numL && !numL->getIsFloat() && numL->getIntVal() == 1) {
            stats.algebraicSimplifications++;
            return rhs;
        }
        // x * 0 -> 0
        if (op == '*' && ((numR && !numR->getIsFloat() && numR->getIntVal() == 0) ||
                          (numL && !numL->getIsFloat() && numL->getIntVal() == 0))) {
            stats.algebraicSimplifications++;
            return std::make_unique<NumberExpr>(static_cast<int64_t>(0), loc);
        }

        // Boolean constant folding
        auto boolL = dyn_cast<BoolExpr>(lhs.get());
        auto boolR = dyn_cast<BoolExpr>(rhs.get());
        if (boolL && boolR) {
            bool l = boolL->getValue();
            bool r = boolR->getValue();
            if (op == tok_and) {
                stats.constantFolds++;
                return std::make_unique<BoolExpr>(l && r, loc);
            }
            if (op == tok_or) {
                stats.constantFolds++;
                return std::make_unique<BoolExpr>(l || r, loc);
            }
        }

        // Reconstruct binary expression if not folded
        return std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs), loc);
    }

    return expr;
}

std::unique_ptr<StmtAST> Optimizer::optimizeStmt(std::unique_ptr<StmtAST> stmt) {
    if (!stmt) return nullptr;

    if (auto block = dyn_cast<BlockStmt>(stmt.get())) {
        optimizeBlock(*block);
        return stmt;
    }

    if (auto vdecl = dyn_cast<VarDeclStmt>(stmt.get())) {
        if (vdecl->getInit()) {
            auto optInit = optimizeExpr(vdecl->takeInit());
            vdecl->setInit(std::move(optInit));
        }
        return stmt;
    }

    if (auto ifStmt = dyn_cast<IfStmt>(stmt.get())) {
        if (ifStmt->getCond()) {
            auto optCond = optimizeExpr(ifStmt->takeCond());
            // If condition folded to constant true or false
            if (auto b = dyn_cast<BoolExpr>(optCond.get())) {
                if (b->getValue()) {
                    // if (true) thenStmt -> replace with thenStmt
                    stats.deadBranchesEliminated++;
                    auto thenBranch = optimizeStmt(ifStmt->takeThenStmt());
                    return thenBranch;
                } else {
                    // if (false) thenStmt else elseStmt -> replace with elseStmt
                    stats.deadBranchesEliminated++;
                    if (ifStmt->getElseStmt()) {
                        auto elseBranch = optimizeStmt(ifStmt->takeElseStmt());
                        return elseBranch;
                    }
                    return nullptr; // pruned
                }
            }
            ifStmt->setCond(std::move(optCond));
        }
        return stmt;
    }

    return stmt;
}

void Optimizer::optimizeBlock(BlockStmt &block) {
    std::vector<std::unique_ptr<StmtAST>> newStmts;
    bool foundReturn = false;

    for (auto &stmt : block.getStatements()) {
        if (foundReturn) {
            stats.deadStatementsEliminated++;
            continue;
        }

        if (isa<ReturnStmt>(stmt.get())) {
            foundReturn = true;
            newStmts.push_back(std::move(stmt));
            continue;
        }

        auto optStmt = optimizeStmt(std::move(stmt));
        if (optStmt) {
            newStmts.push_back(std::move(optStmt));
        }
    }

    block.getStatements() = std::move(newStmts);
}

} // namespace algoc
