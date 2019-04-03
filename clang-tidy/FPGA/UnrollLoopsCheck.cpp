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
  // checkNeedsUnrolling(MatchedLoop, Context);
  UnrollType unroll = unrollType(MatchedLoop, Result.Context);
  if (unroll == NotUnrolled) {
    diag(MatchedLoop->getBeginLoc(), "The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive");
    return;
  }
  if (unroll == PartiallyUnrolled) {
    diag(MatchedLoop->getBeginLoc(), "This loop is partially unrolled, all's good");
    return;
  }
  if (unroll == FullyUnrolled) {
    if (hasKnownBounds(MatchedLoop)) {
      if (hasLargeNumIterations(MatchedLoop, Context)) {
        diag(MatchedLoop->getBeginLoc(), "This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive");
        return;
      }
      diag(MatchedLoop->getBeginLoc(), "Full unrolling should be successful. All good.");
      return;
    }
    diag(MatchedLoop->getBeginLoc(), "Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, using the #pragma unroll <num> directive");
  }
}

// void UnrollLoopsCheck::checkNeedsUnrolling(const Stmt *Statement, ASTContext *Context) {
// }

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

bool UnrollLoopsCheck::hasKnownBounds(const Stmt* Statement) {
  const Expr *condExpr = getCondExpr(Statement);
  if (!condExpr) {
    return false; //diag(Statement->getBeginLoc(), statementClassName);
  } 
  if (std::string(condExpr->getStmtClassName()).compare("BinaryOperator") == 0) {
    const BinaryOperator* binaryOp = static_cast<const BinaryOperator*>(condExpr);
    const Expr* lhs = binaryOp->getLHS();
    const Expr* rhs = binaryOp->getRHS();
    if (lhs->isValueDependent() && rhs->isValueDependent()) {
      diag(binaryOp->getExprLoc(), "Has two value-dependent sides");
      return false;  // Both sides are value dependent, so we don't know the loop bounds.
    }
    if (!(lhs->isValueDependent()) && !(rhs->isValueDependent())) {
      diag(binaryOp->getExprLoc(), "Has two constant valued sides");
      return false;  // Both sides are constant, so it's likely an infinite loop.
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
    return false; //diag(Statement->getBeginLoc(), statementClassName);
  } 
  if (std::string(condExpr->getStmtClassName()).compare("BinaryOperator") == 0) {
    const BinaryOperator* binaryOp = static_cast<const BinaryOperator*>(condExpr);
    const Expr* lhs = binaryOp->getLHS();
    const Expr* rhs = binaryOp->getRHS();
    Expr::EvalResult result;
    // const ASTContext context = ASTContext(&Context);
    if (lhs->isValueDependent() && !(rhs->isValueDependent())) {
      if (rhs->EvaluateAsRValue(result, *Context)) {
        if (!(result.Val.isInt())) {
          return false;  // Cannot check number of iterations, return false to be safe
        }
        if (result.Val.getInt() > loop_iterations) {
          return true;  // Assumes values go from 0 to Val in increments of 1
        }
        return false;  // Number of iterations likely less than option
      }
    }
    if (rhs->isValueDependent() && !(lhs->isValueDependent())) {
      if (lhs->EvaluateAsRValue(result, *Context)) {
        if (!(result.Val.isInt())) {
          return false;  // Cannot check number of iterations, return false to be safe
        }
        if (result.Val.getInt() > loop_iterations) {
          return true;  // Assumes values go from 0 to Val in increments of 1
        }
        return false;  // Number of iterations likely less than option
      }
    }
  }
  return false;  // Cannot check number of iteration, return false to be safe
}

void UnrollLoopsCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "loop_iterations", loop_iterations);
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
