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
#include "PossiblyUnreachableBarrierCheck.h"
#include "RecursionNotSupportedCheck.h"


using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace OpenCL {

class OpenCLModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<PossiblyUnreachableBarrierCheck>(
        "OpenCL-possibly-unreachable-barrier");
    CheckFactories.registerCheck<RecursionNotSupportedCheck>(
        "OpenCL-recursion-not-supported");
  }
};

static ClangTidyModuleRegistry::Add<OpenCLModule> X("opencl-module", "Adds General OpenCL lint checks.");

} // namespace OpenCL

// This anchor is used to force the linker to link in the generated object file
// and thus register the MyModule.
volatile int OpenCLModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
