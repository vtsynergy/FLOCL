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
const unsigned loop_iterations;

public:
  UnrollLoopsCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context),
    loop_iterations(Options.get("loop_iterations", 1000U)) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
private:
  /// The kind of unrolling, if any, applied to a given loop
  enum UnrollType { 
    NotUnrolled, // This loop has no #pragma unroll directive associated with it
    FullyUnrolled, // This loop has a #pragma unroll directive associated with it
    PartiallyUnrolled // This loop has a #pragma unroll <num> directive associated with it
  };
  /// Performs the check that determines whether or not a loop statement
  /// needs unrolling, and prints the diagnostic message if it does
  void checkNeedsUnrolling(const Stmt* Statement, ASTContext *Context);
  /// Returns the type of unrolling, if any, associated with the given 
  /// statement.
  enum UnrollType unrollType(const Stmt* Statement, ASTContext *Context);
  /// Returns the condition expression within a given for statement. If there is none,
  /// or if the Statement is not a loop, then returns a NULL pointer.
  const Expr* getCondExpr(const Stmt* Statement);
  /// Returns True if the loop statement has known bounds
  bool hasKnownBounds(const Stmt* Statement);
  void storeOptions(ClangTidyOptions::OptionMap &Opts);
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_UNROLLLOOPSCHECK_H
