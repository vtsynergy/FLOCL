//===--- RecursionNotSupportedCheck.h - clang-tidy --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_RECURSIONNOTSUPPORTEDCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_RECURSIONNOTSUPPORTEDCHECK_H

#include <map>
#include <vector>
#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace OpenCL {

/// Flags instances of recurrent function calls as errors.
///
/// This lint check makes use of a recurrent function to find instances of
/// recursive function calls. To reduce the impact of this lint check on the
/// linting time, an option parameter MaxRecursionDepth is used to set how
/// deep the recursive function goes to look for recursive function calls.
///
/// For the user-facing documentation see:
/// clang.llvm.org/extra/clang-tidy/checks/OpenCL-recursion-not-supported.html
class RecursionNotSupportedCheck : public ClangTidyCheck {
const unsigned MaxRecursionDepth;

public:
  RecursionNotSupportedCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context),
    MaxRecursionDepth(Options.get("MaxRecursionDepth", 5U)) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
private:
  /// Stores the names of all the callers of each function, as well as the 
  /// location at which the function calls are made.
  std::map<std::string, 
      std::vector<std::pair<SourceLocation,std::string>>> Callers;
  /// Stores the source location ranges of every function declaration.
  std::map<std::string, SourceRange> Locations;
  /// Stores the source location range of the newly matched function
  /// declaration.
  void handleFunctionDecl(const FunctionDecl *FunDecl);
  /// Stores the caller of the newly matched function call, and performs the
  /// check for whether or not the function call is recursive.
  void handleFunctionCall(const DeclRefExpr *FunCall, const SourceManager *SM);
  /// Checks if the current function call is recursive, and returns the
  /// recursion path as a string parameter. If the function call is not
  /// recursive, returns an empty string.
  std::string isRecursive(std::string &FunCallName, std::string &CallerName,
      unsigned Depth, const SourceManager *SM);
  /// Helper function that builds a portion of the recursion path. 
  std::string buildStringPath(std::string &FunCallName, std::string &CallerName,
      unsigned Depth, const SourceManager *SM, SourceLocation Loc);
  void storeOptions(ClangTidyOptions::OptionMap &Opts);
};

} // namespace OpenCL
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_RECURSIONNOTSUPPORTEDCHECK_H
