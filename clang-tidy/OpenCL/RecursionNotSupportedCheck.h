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

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/OpenCL-recursion-not-supported.html
class RecursionNotSupportedCheck : public ClangTidyCheck {
public:
  RecursionNotSupportedCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
private:
  std::map<std::string, std::vector<std::string>> Callees;
  std::map<std::string, SourceRange> Locations;
  void handleFunctionDecl(const FunctionDecl *functionDecl);
  void handleFunctionCall(const DeclRefExpr *functionCall);
  bool isCalledIn(const DeclRefExpr *functionCall, std::string &functionName);
};

} // namespace OpenCL
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_RECURSIONNOTSUPPORTEDCHECK_H
