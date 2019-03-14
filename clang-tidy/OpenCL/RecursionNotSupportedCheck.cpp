//===--- RecursionNotSupportedCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RecursionNotSupportedCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace OpenCL {

void RecursionNotSupportedCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(functionDecl().bind("functionDecl"), this);
  Finder->addMatcher(declRefExpr(to(functionDecl())).bind("functionCall"), this);
}

void RecursionNotSupportedCheck::check(const MatchFinder::MatchResult &Result) {
  const FunctionDecl *MatchedFunDecl = Result.Nodes.getNodeAs<FunctionDecl>("functionDecl");
  const DeclRefExpr *MatchedFunCall = Result.Nodes.getNodeAs<DeclRefExpr>("functionCall");

  if (MatchedFunDecl) {
    handleFunctionDecl(MatchedFunDecl);
  } else {
    std::string functionCallName = MatchedFunCall->getNameInfo().getName().getAsString();
    diag(MatchedFunCall->getLocation(), "functionCall is made here");
  }
  
}

void RecursionNotSupportedCheck::handleFunctionDecl(const FunctionDecl *functionDecl) {
  std::string functionDeclName = functionDecl->getNameInfo().getName().getAsString();
  diag(functionDecl->getLocation(), "function %0 is declared here")
      << functionDecl;
  Locations[functionDeclName] = functionDecl->getSourceRange();
}

void RecursionNotSupportedCheck::handleFunctionCall(const DeclRefExpr *functionCall) {

}

bool RecursionNotSupportedCheck::isCalledIn(const DeclRefExpr *functionCall, std::string &functionName) {

}

} // namespace OpenCL
} // namespace tidy
} // namespace clang
