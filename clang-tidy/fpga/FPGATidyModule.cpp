//===--- FPGATidyModule.cpp - clang-tidy ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "StructPackAlignCheck.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

class FPGAModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<StructPackAlignCheck>(
        "fpga-struct-pack-align");
  }
};

static ClangTidyModuleRegistry::Add<FPGAModule>
    X("fpga-module", "Adds Altera FPGA OpenCL lint checks.");

} // namespace FPGA

// This anchor is used to force the linker to link in the generated object file
// and thus register the MyModule.
volatile int FPGAModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
