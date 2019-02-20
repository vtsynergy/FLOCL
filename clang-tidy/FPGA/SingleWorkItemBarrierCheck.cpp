//===--- SingleWorkItemBarrierCheck.cpp - clang-tidy-----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SingleWorkItemBarrierCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

void SingleWorkItemBarrierCheck::registerMatchers(MatchFinder *Finder) {
  // Find any function that calls barrier but does not call either get_local_id or get_global_id, and will thus be treated as a single-work-item kernel
  //hasAttr(attr::Kind::OpenCLKernel) restricts it to only kernel functions
  // TODO: Have it accept all functions but check for a parameter that gets an ID from one of the two ID functions
  Finder->addMatcher(
    //Find function declarations
    functionDecl(allOf(
      //That are OpenCL kernels
      hasAttr(attr::Kind::OpenCLKernel),
      //And call a barrier function (either 1.x or 2.x version)
      //hasDescendant(callExpr(callee(
      forEachDescendant(callExpr(callee(
        functionDecl(anyOf(
          hasName("barrier"),
          hasName("work_group_barrier")
        ))
      )).bind("barrier")),
      //But do not call an ID function
      unless(hasDescendant(callExpr(callee(
        functionDecl(anyOf(
          hasName("get_global_id"),
          hasName("get_local_id")
        ))
      ))))
    )).bind("function"), this);
}

void SingleWorkItemBarrierCheck::check(const MatchFinder::MatchResult &Result) {
  //TODO: Support reqd_work_group_size attribute (if it is anything other than (1,1,1) it will be interpreted as an NDRange (at least in 17.1, don't know about earlier)
  const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("function");
  const auto *MatchedBarrier = Result.Nodes.getNodeAs<CallExpr>("barrier");
  if (aoc_version < 1701) {
    diag(MatchedDecl->getLocation(), "Kernel function %0 does not call get_global_id or get_local_id and will be treated as single-work-item.\nBarrier call at %1 may error out")
      << MatchedDecl
      << MatchedBarrier->getBeginLoc().printToString(Result.Context->getSourceManager());
  } else {
    diag(MatchedDecl->getLocation(), "Kernel function %0 does not call get_global_id or get_local_id may be a viable single work-item kernel, but barrier call at %1 will force NDRange execution. If single work-item semantics are desired a mem_fence may be more efficient.")
      << MatchedDecl
      << MatchedBarrier->getBeginLoc().printToString(Result.Context->getSourceManager());

  }
}

void SingleWorkItemBarrierCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "aoc_version", aoc_version);
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
