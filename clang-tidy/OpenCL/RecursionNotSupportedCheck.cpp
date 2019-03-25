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
    handleFunctionCall(MatchedFunCall, Result.SourceManager);
    // std::string functionCallName = MatchedFunCall->getNameInfo().getName().getAsString();
    // diag(MatchedFunCall->getLocation(), "functionCall is made here");
  }
  
}

void RecursionNotSupportedCheck::handleFunctionDecl(const FunctionDecl *functionDecl) {
  std::string functionDeclName = functionDecl->getNameInfo().getName().getAsString();
  diag(functionDecl->getLocation(), "function %0 is declared here")
      << functionDeclName;
  Locations[functionDeclName] = functionDecl->getSourceRange();
}

void RecursionNotSupportedCheck::handleFunctionCall(const DeclRefExpr *functionCall, const SourceManager *sourceManager) {
  std::string functionCallName = functionCall->getNameInfo().getName().getAsString();
  diag(functionCall->getLocation(), "function %0 is called here")
      << functionCallName;
  auto iter = Callees.find(functionCallName);
  if (iter == Callees.end()) {  // First instance of a call to this function
    Callees[functionCallName] = std::vector<std::string>();
  }
  // TODO: Find all callees of said function
  for (auto functionDecl = Locations.begin(); functionDecl != Locations.end(); functionDecl++) {
    if (isPointWithin(functionCall->getLocation(), functionDecl->second.getBegin(), functionDecl->second.getEnd())) {
      Callees[functionCallName].push_back(functionDecl->first);
    }
  }
}

bool RecursionNotSupportedCheck::isCalledIn(const DeclRefExpr *functionCall, std::string &functionName) {

}

} // namespace OpenCL
} // namespace tidy
} // namespace clang
