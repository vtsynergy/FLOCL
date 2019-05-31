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
/// Also attempts to finds for, while and do..while loops that have been fully 
/// unrolled, but either have unknown bounds or a large number of iterations.
/// The compiler will not unroll these loops, so partial unrolling is
/// recommended in these cases.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/FPGA-unroll-loops.html
class UnrollLoopsCheck : public ClangTidyCheck {
const unsigned max_loop_iterations;

public:
  UnrollLoopsCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context),
    max_loop_iterations(Options.get("max_loop_iterations", 100U)) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
private:
  /// The kind of unrolling, if any, applied to a given loop
  enum UnrollType { 
    NotUnrolled, // This loop has no #pragma unroll directive associated with it
    FullyUnrolled, // This loop has a #pragma unroll directive associated with it
    PartiallyUnrolled // This loop has a #pragma unroll <num> directive associated with it
  };
  /// Returns true if the given loop statement has a large number of iterations,
  /// as determined by the integer value in the loop's condition expression, 
  /// if one exists.
  bool hasLargeNumIterations(const Stmt* Statement, const ASTContext* Context);
  /// Checks one hand side of the binary operator to ascertain if the upper
  /// bound on the number of loops is greater than max_loop_iterations or not.
  /// If the expression is not evaluatable or not an integer, returns false.
  bool exprHasLargeNumIterations(const Expr* expr, const ASTContext* Context);
  /// Returns the type of unrolling, if any, associated with the given 
  /// statement.
  enum UnrollType unrollType(const Stmt* Statement, ASTContext *Context);
  /// Returns the condition expression within a given for statement. If there is none,
  /// or if the Statement is not a loop, then returns a NULL pointer.
  const Expr* getCondExpr(const Stmt* Statement);
  /// Returns True if the loop statement has known bounds
  bool hasKnownBounds(const Stmt* Statement, const ASTContext* Context);
  void storeOptions(ClangTidyOptions::OptionMap &Opts);
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_UNROLLLOOPSCHECK_H
