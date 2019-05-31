//===--- UnrollLoopsCheck.cpp - clang-tidy --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UnrollLoopsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

void UnrollLoopsCheck::registerMatchers(MatchFinder *Finder) {
  const auto ANYLOOP = anyOf(forStmt(), whileStmt(), doStmt());
  Finder->addMatcher(stmt(
            allOf(
              ANYLOOP,  // Match all loop types,
              unless(hasDescendant(forStmt())),
              unless(hasDescendant(whileStmt())),
              unless(hasDescendant(doStmt())))).bind("loop"), this);
}

void UnrollLoopsCheck::check(const MatchFinder::MatchResult &Result) {
  const Stmt *MatchedLoop = Result.Nodes.getNodeAs<Stmt>("loop");
  const ASTContext *Context = Result.Context;
  UnrollType unroll = unrollType(MatchedLoop, Result.Context);
  if (unroll == NotUnrolled) {
    diag(MatchedLoop->getBeginLoc(), "The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive");
    return;
  }
  if (unroll == PartiallyUnrolled) {
    return;
  }
  if (unroll == FullyUnrolled) {
    if (hasKnownBounds(MatchedLoop, Context)) {
      if (hasLargeNumIterations(MatchedLoop, Context)) {
        diag(MatchedLoop->getBeginLoc(), "This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive");
        return;
      }
      return;
    }
    diag(MatchedLoop->getBeginLoc(), "Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive");
  }
}

enum UnrollLoopsCheck::UnrollType UnrollLoopsCheck::unrollType(const Stmt *Statement, ASTContext *Context) {
  const auto parents = Context->getParents(*Statement);
  for (size_t i = 0; i < parents.size(); ++i) {
    const auto &parent = parents[i];
    if (parent.getNodeKind().asStringRef().equals("AttributedStmt")) {
      const AttributedStmt *parentStmt = parent.get<AttributedStmt>();
      for (size_t j = 0; j < parentStmt->getAttrs().size(); ++j) {
        auto *attr = parentStmt->getAttrs()[j];
        const LoopHintAttr *loopHintAttr;
        if ((loopHintAttr = static_cast<const LoopHintAttr*>(attr))) {
          if (loopHintAttr->getState() == LoopHintAttr::Numeric) {
            return PartiallyUnrolled;
          }
          if (loopHintAttr->getState() == LoopHintAttr::Disable) {
            return NotUnrolled;
          }
          if (loopHintAttr->getState() == LoopHintAttr::Full) {
            return FullyUnrolled;
          }
          if (loopHintAttr->getState() == LoopHintAttr::Enable) {
            return FullyUnrolled;
          }
        }
      }
    }
  }
  return NotUnrolled;
}

bool UnrollLoopsCheck::hasKnownBounds(const Stmt* Statement, const ASTContext* Context) {
  const Expr *condExpr = getCondExpr(Statement);
  if (!condExpr) {
    return false; //diag(Statement->getBeginLoc(), statementClassName);
  } 
  if (std::string(condExpr->getStmtClassName()).compare("BinaryOperator") == 0) {
    const BinaryOperator* binaryOp = static_cast<const BinaryOperator*>(condExpr);
    const Expr* lhs = binaryOp->getLHS();
    const Expr* rhs = binaryOp->getRHS();
    if (lhs->isEvaluatable(*Context) == rhs->isEvaluatable(*Context)) {
      return false;  // Both sides are value dependent or constant, so loop bounds are not known.
    }
    return true;  // At least 1 side isn't value dependent, so we know the loop bounds.
  }
  return false;  // If it's not a binary operator, we don't know the loop bounds.
}

const Expr* UnrollLoopsCheck::getCondExpr(const Stmt* Statement) {
  const Expr *condExpr;
  std::string statementClassName = Statement->getStmtClassName();
  if (statementClassName.compare("ForStmt") == 0) {
    const ForStmt *forStmt = static_cast<const ForStmt*>(Statement);
    condExpr = forStmt->getCond();
  }
  if (statementClassName.compare("WhileStmt") == 0) {
    const WhileStmt *whileStmt = static_cast<const WhileStmt*>(Statement);
    condExpr = whileStmt->getCond();
  }
  if (statementClassName.compare("DoStmt") == 0) {
    const DoStmt *doStmt = static_cast<const DoStmt*>(Statement);
    condExpr = doStmt->getCond();
  }
  return condExpr;
}

bool UnrollLoopsCheck::hasLargeNumIterations(const Stmt* Statement, const ASTContext* Context) {
  const Expr *condExpr = getCondExpr(Statement);
  if (!condExpr) {
    return false;
  } 
  if (std::string(condExpr->getStmtClassName()).compare("BinaryOperator") == 0) {
    const BinaryOperator* binaryOp = static_cast<const BinaryOperator*>(condExpr);
    const Expr* lhs = binaryOp->getLHS();
    const Expr* rhs = binaryOp->getRHS();
    Expr::EvalResult result;
    if (lhs->isEvaluatable(*Context) && !(rhs->isEvaluatable(*Context))) {
      return exprHasLargeNumIterations(lhs, Context);
    }
    if (rhs->isEvaluatable(*Context) && !(lhs->isEvaluatable(*Context))) {
      return exprHasLargeNumIterations(rhs, Context);
    }
  }
  return false;  // Cannot check number of iteration, return false to be safe
}

bool UnrollLoopsCheck::exprHasLargeNumIterations(const Expr* expr, const ASTContext* Context) {
  Expr::EvalResult result;
  if (expr->EvaluateAsRValue(result, *Context)) {
    if (!(result.Val.isInt())) {
      return false;  // Cannot check number of iterations, return false to be safe
    }
    if (result.Val.getInt() > max_loop_iterations) {
      return true;  // Assumes values go from 0 to Val in increments of 1
    }
    return false;  // Number of iterations likely less than max_loop_iterations
  }
  return false;  // Cannot evaluate as an r-value, so cannot check number of iterations
}

void UnrollLoopsCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "max_loop_iterations", max_loop_iterations);
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
