//===--- UnrollLoopsCheck.h - clang-tidy ------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_UNROLLLOOPSCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_UNROLLLOOPSCHECK_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace FPGA {

/// Finds for, while and do..while loop statements that have not been unrolled.
/// Unrolling these loops could increase the performance of the OpenCL kernel.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/FPGA-unroll-loops.html
class UnrollLoopsCheck : public ClangTidyCheck {
public:
  UnrollLoopsCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
private:
  /// Performs the check that determines whether or not a loop statement
  /// needs unrolling, and prints the diagnostic message if it does
  void checkNeedsUnrolling(const Stmt* Statement, ASTContext *Context);
  /// Returns True if the given statement does not have a parent that's an
  /// AttributedStmt with an attribute named "unroll".
  bool needsUnrolling(const Stmt* Statement, ASTContext *Context);
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_UNROLLLOOPSCHECK_H
