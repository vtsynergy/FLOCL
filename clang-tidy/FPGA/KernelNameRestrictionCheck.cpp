//===--- KernelNameRestrictionCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "KernelNameRestrictionCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

void KernelNameRestrictionCheck::registerMatchers(MatchFinder *Finder) {
  // TranslationUnitDecl is the root of all ASTs
  Finder->addMatcher(functionDecl().bind("function"), this);
}

void KernelNameRestrictionCheck::check(const MatchFinder::MatchResult &Result) {
  // StackOverflow example: https://stackoverflow.com/a/25079453/5760608
  const Decl *MatchedDecl = Result.Nodes.getNodeAs<Decl>("function");
  ASTContext &Context = MatchedDecl->getASTContext();
  const SourceManager &SrcManager = Context.getSourceManager();
  const FileEntry *Entry = SrcManager.getFileEntryForID(
    SrcManager.getFileID(
      MatchedDecl->getBeginLoc()));
  StringRef FileName = Entry->getName();
  if (FileName.endswith_lower("/kernel.cl") || FileName.endswith_lower("/verilog.cl") || FileName.endswith_lower("/vhdl.cl")) {
    diag(SrcManager.translateLineCol(SrcManager.getFileID(MatchedDecl->getBeginLoc()),1,1),
       "Naming your OpenCL kernel source file \"kernel.cl\", \"Verilog.cl\", or \"VHDL.cl\" could cause compilation errors.");
    // diag(MatchedDecl->getLocation(), "Naming your OpenCL kernel source file \"kernel.cl\" could cause compilation errors.");
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
