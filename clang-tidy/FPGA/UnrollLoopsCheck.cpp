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
  ASTContext *Context = Result.Context;
  checkNeedsUnrolling(MatchedLoop, Context);
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

void UnrollLoopsCheck::checkNeedsUnrolling(const Stmt *Statement, ASTContext *Context) {
  UnrollType unroll = unrollType(Statement, Context);
  if (unroll == NotUnrolled) {
    diag(Statement->getBeginLoc(), "The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive");
  }
  if (unroll == PartiallyUnrolled) {
    diag(Statement->getBeginLoc(), "This loop is partially unrolled, all's good");
  }
  if (unroll == FullyUnrolled) {
    if (!hasKnownBounds(Statement)) {
      diag(Statement->getBeginLoc(), "Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, using the #pragma unroll <num> directive");
    } else {
      diag(Statement->getBeginLoc(), "This loop is fully unrolled, check for edge cases");
    }
  }
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

void UnrollLoopsCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "loop_iterations", loop_iterations);
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
