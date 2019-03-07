//===--- SingleWorkItemBarrierCheck.h - clang-tidy---------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_SINGLE_WORK_ITEM_BARRIER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_SINGLE_WORK_ITEM_BARRIER_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace FPGA {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/OpenCL-single-work-item-barrier.html
class SingleWorkItemBarrierCheck : public ClangTidyCheck {
const unsigned aoc_version;

public:
  SingleWorkItemBarrierCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context),
	aoc_version(Options.get("aoc_version", 1600U)) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_SINGLE_WORK_ITEM_BARRIER_H
