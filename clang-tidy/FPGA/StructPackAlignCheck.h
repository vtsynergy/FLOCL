//===--- StructPackAlignCheck.h - clang-tidy---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_STRUCT_PACK_ALIGN_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_STRUCT_PACK_ALIGN_H

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

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_STRUCT_PACK_ALIGN_H
