//===--- PossiblyUnreachableBarrierCheck.h - clang-tidy----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_POSSIBLY_UNREACHABLE_BARRIER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_POSSIBLY_UNREACHABLE_BARRIER_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace OpenCL {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/OpenCL-possibly-unreachable-barrier.html
class PossiblyUnreachableBarrierCheck : public ClangTidyCheck {
private:
  std::vector<const VarDecl *> IDDepVars;
  std::vector<const FieldDecl *> IDDepFields;
public:
  PossiblyUnreachableBarrierCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  const DeclRefExpr * hasIDDepDeclRef(const Expr * e);
  const MemberExpr * hasIDDepMember(const Expr * e);
  void preorderFlattenStmt(const Stmt * s, std::list<const Stmt *> * out);
};

} // namespace OpenCL
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENCL_POSSIBLY_UNREACHABLE_BARRIER_H
