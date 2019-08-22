//===--- OpenCLTidyModule.cpp - clang-tidy --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "IdDependentBackwardBranchCheck.h"
#include "KernelNameRestrictionCheck.h"
#include "SingleWorkItemBarrierCheck.h"
#include "StructPackAlignCheck.h"
#include "UnrollLoopsCheck.h"


using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

class FPGAModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<IdDependentBackwardBranchCheck>(
        "fpga-id-dependent-backward-branch");
    CheckFactories.registerCheck<KernelNameRestrictionCheck>(
        "fpga-kernel-name-restriction");
    CheckFactories.registerCheck<SingleWorkItemBarrierCheck>(
        "fpga-single-work-item-barrier");
    CheckFactories.registerCheck<StructPackAlignCheck>(
        "fpga-struct-pack-align");
    CheckFactories.registerCheck<UnrollLoopsCheck>(
        "fpga-unroll-loops");
  }
};

static ClangTidyModuleRegistry::Add<FPGAModule> X("fpga-module", "Adds Altera FPGA OpenCL lint checks.");

} // namespace flocl

// This anchor is used to force the linker to link in the generated object file
// and thus register the MyModule.
volatile int FPGAModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
