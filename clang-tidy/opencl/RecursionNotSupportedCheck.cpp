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
  Finder->addMatcher(
      declRefExpr(to(functionDecl())).bind("functionCall"), this);
}

void RecursionNotSupportedCheck::check(const MatchFinder::MatchResult &Result) {
  auto MatchedFunDecl = Result.Nodes.getNodeAs<FunctionDecl>("functionDecl");
  auto MatchedFunCall = Result.Nodes.getNodeAs<DeclRefExpr>("functionCall");

  if (MatchedFunDecl) {
    handleFunctionDecl(MatchedFunDecl);
  } else {
    handleFunctionCall(MatchedFunCall, Result.SourceManager);
  }
  
}

void RecursionNotSupportedCheck::handleFunctionDecl(
    const FunctionDecl *FunDecl) {
  std::string FunDeclName = FunDecl->getNameInfo().getName().getAsString();
  Locations[FunDeclName] = FunDecl->getSourceRange();
}

void RecursionNotSupportedCheck::handleFunctionCall(const DeclRefExpr *FunCall,
    const SourceManager *SM) {
  std::string FunCallName = FunCall->getNameInfo().getName().getAsString();
  // Update Callees map
  auto Iter = Callers.find(FunCallName);
  if (Iter == Callers.end()) {  // First instance of a call to this function
    Callers[FunCallName] = std::vector<std::pair<SourceLocation,std::string>>();
  }
  for (auto const &I : Locations) {
    if (SM->isPointWithin(FunCall->getLocation(), I.second.getBegin(), 
        I.second.getEnd())) {
      Callers[FunCallName].push_back(std::make_pair(FunCall->getBeginLoc(),
                                     I.first));
    }
  }
  // Check if function call is recursive
  std::string RecursivePath = isRecursive(FunCallName, FunCallName, 0, SM);
  if (!RecursivePath.empty()) {
    diag(FunCall->getBeginLoc(), 
         "The call to function %0 is recursive, which is not supported by "
         "OpenCL.\n%1", DiagnosticIDs::Error)
        << FunCallName << RecursivePath;
  }
}

std::string RecursionNotSupportedCheck::isRecursive(std::string &FunCallName, 
    std::string &CallerName, unsigned Depth, const SourceManager *SM) {
  if (Depth == MaxRecursionDepth) {
    return "";
  }
  for(std::pair<SourceLocation,std::string> &Caller : Callers[CallerName]) {
    if (Caller.second.compare(FunCallName) == 0) {
      return buildStringPath(CallerName, FunCallName, SM, Caller.first);
    }
    std::string StringPath = isRecursive(FunCallName, Caller.second, Depth+1,
                                         SM);
    if (!StringPath.empty()) {
      std::ostringstream StringStream;
      StringStream << buildStringPath(CallerName, Caller.second, SM,
                                      Caller.first) 
          << "\n" << StringPath;
      return StringStream.str();
    }
  }
  return "";
}

std::string RecursionNotSupportedCheck::buildStringPath(
    std::string &FunCallName, std::string &CallerName, const SourceManager *SM,
    SourceLocation Loc) {
  std::ostringstream StringStream;
  StringStream << "\t";
  std::pair<FileID, unsigned> FileOffset = SM->getDecomposedLoc(Loc);
  std::string FilePath = SM->getFileEntryForID(
      FileOffset.first)->tryGetRealPathName();
  unsigned LineNum = SM->getLineNumber(FileOffset.first, FileOffset.second);
  unsigned ColNum = SM->getColumnNumber(FileOffset.first, FileOffset.second);
  StringStream << FunCallName << " is called by " << CallerName << " in "
      << FilePath << ":" << LineNum << ":" << ColNum;
  std::string StringPath = StringStream.str();
  return StringPath;
}

void RecursionNotSupportedCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "MaxRecursionDepth", MaxRecursionDepth);
}

} // namespace OpenCL
} // namespace tidy
} // namespace clang
