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
#include <sstream>

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
  }
  
}

void RecursionNotSupportedCheck::handleFunctionDecl(const FunctionDecl *functionDecl) {
  std::string functionDeclName = functionDecl->getNameInfo().getName().getAsString();
  Locations[functionDeclName] = functionDecl->getSourceRange();
}

void RecursionNotSupportedCheck::handleFunctionCall(const DeclRefExpr *functionCall, const SourceManager *sourceManager) {
  std::string functionCallName = functionCall->getNameInfo().getName().getAsString();
  // Update Callees map
  auto iter = Callers.find(functionCallName);
  if (iter == Callers.end()) {  // First instance of a call to this function
    Callers[functionCallName] = std::vector<std::pair<SourceLocation,std::string>>();
  }
  for (auto functionDecl = Locations.begin(); functionDecl != Locations.end(); functionDecl++) {
    if (sourceManager->isPointWithin(functionCall->getLocation(), functionDecl->second.getBegin(), functionDecl->second.getEnd())) {
      Callers[functionCallName].push_back(std::make_pair(functionCall->getBeginLoc(), functionDecl->first));
    }
  }
  // Check if function call is recursive
  std::string recursivePath = isRecursive(functionCallName, functionCallName, 0);
  if (!recursivePath.empty()) {
    diag(functionCall->getBeginLoc(), "The call to function %0 is recursive, which is not supported by OpenCL.", DiagnosticIDs::Error)
        << functionCallName;
    diag(functionCall->getBeginLoc(), recursivePath, DiagnosticIDs::Note);
  }
}

std::string RecursionNotSupportedCheck::isRecursive(std::string &functionCallName, std::string &callerName, int depth) {
  if (depth == 5) {
    return "";
  }
  for(std::pair<SourceLocation,std::string> &caller: Callers[callerName]) {
    if (caller.second.compare(functionCallName) == 0) {
      // Try adding note here
      return buildStringPath(callerName, functionCallName, depth);
    }
    std::string stringPath = isRecursive(functionCallName, caller.second, depth+1);
    if (!stringPath.empty()) {
      std::ostringstream stringStream;
      stringStream << buildStringPath(callerName, caller.second, depth) << std::endl << stringPath;
      return stringStream.str();
    }
  }
  return "";
}

std::string RecursionNotSupportedCheck::buildStringPath(std::string &functionCallName, std::string &callerName, int depth) {
  std::ostringstream stringStream;
  for (int i = 0; i < depth+1; ++i) {
    stringStream << "\t";
  }
  stringStream << "function " << functionCallName << " is called by function " << callerName;
  std::string stringPath = stringStream.str();
  return stringPath;
}


} // namespace OpenCL
} // namespace tidy
} // namespace clang
