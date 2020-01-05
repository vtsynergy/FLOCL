//===--- KernelNameRestrictionCheck.h - clang-tidy --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_ALTERA_KERNEL_NAME_RESTRICTION_CHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_ALTERA_KERNEL_NAME_RESTRICTION_CHECK_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace altera {

/// Flags kernel files that are named `kernel.cl`, `Verilog.cl` or `VHDL.cl` as
/// warnings, since this may lead to intermediate files being generated that
/// have the same names as certain internal files, leading to compiler errors.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/altera-kernel-name-restriction.html
class KernelNameRestrictionCheck : public ClangTidyCheck {
public:
  KernelNameRestrictionCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                           Preprocessor *ModuleExpanderPP) override;
};

} // namespace altera
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_ALTERA_KERNEL_NAME_RESTRICTION_CHECK_H
