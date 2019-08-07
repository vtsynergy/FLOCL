//===--- IdDependentBackwardBranchCheck.h - clang-tidy-----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_ID_DEPENDENT_BACKWARD_BRANCH_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_ID_DEPENDENT_BACKWARD_BRANCH_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace FPGA {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/FPGA-ID-dependent-backward-branch.html
class IdDependentBackwardBranchCheck : public ClangTidyCheck {
private:
  std::vector<const VarDecl *> IDDepVars;
  std::vector<const FieldDecl *> IDDepFields;
  std::map<std::string, std::pair<SourceLocation, std::string>> IDDepVarsMap;
  std::map<std::string, std::pair<const FieldDecl *, std::string>> IDDepFieldsMap;
public:
  IdDependentBackwardBranchCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  const DeclRefExpr * hasIDDepDeclRef(const Expr * e);
  const MemberExpr * hasIDDepMember(const Expr * e);
  std::pair<std::string, std::vector<std::pair<SourceLocation, std::string>>> hasIDDepVar(const Expr * Expression);
  std::string hasIDDepField(const Expr * Expression);
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_ID_DEPENDENT_BACKWARD_BRANCH_H
