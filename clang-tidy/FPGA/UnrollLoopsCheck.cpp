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
        if (loopHintAttr = static_cast<const LoopHintAttr*>(attr)) {
          if (loopHintAttr->getState() == LoopHintAttr::Numeric) {
            // diag(loopHintAttr->getLocation(), "LoopHintAttr has a numeric state");
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
        
        // diag(attr->getLocation(), typeid(*attr).name());
        // if (StringRef("unroll").equals(attr->getSpelling())) {
        //   return false;
        // }
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
    diag(Statement->getBeginLoc(), "This loop is fully unrolled, check for edge cases");
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
