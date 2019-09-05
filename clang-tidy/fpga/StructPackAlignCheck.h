//===--- StructPackAlignCheck.h - clang-tidy --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_STRUCTPACKALIGNCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_STRUCTPACKALIGNCHECK_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace FPGA {

/// Finds structs that are inefficiently packed or aligned, and recommends
/// packing and/or aligning of said structs as needed.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/FPGA-struct-pack-align.html
class StructPackAlignCheck : public ClangTidyCheck {
public:
  StructPackAlignCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_STRUCTPACKALIGNCHECK_H
