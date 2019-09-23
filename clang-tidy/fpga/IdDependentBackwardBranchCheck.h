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

using VariableUsage = std::vector<std::pair<SourceLocation, std::string>>;

/// Finds ID-dependent variables and fields used within loops, and warns of
/// their usage. Using these variables in loops can lead to performance
/// degradation. 
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/FPGA-ID-dependent-backward-branch.html
class IdDependentBackwardBranchCheck : public ClangTidyCheck {
private:
  // std::vector<const VarDecl *> IDDepVars;
  // std::vector<const FieldDecl *> IDDepFields;
  std::map<const VarDecl *, VariableUsage> IDDepVarsMap;
  std::map<const FieldDecl *, VariableUsage> IDDepFieldsMap;
public:
  IdDependentBackwardBranchCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  const DeclRefExpr * hasIDDepDeclRef(const Expr * e);
  const MemberExpr * hasIDDepMember(const Expr * e);
  void addIDDepVar(const Stmt* Statement, const VarDecl* Variable);
  void addIDDepField(const Stmt* Statement, const FieldDecl* Field);
  std::pair<const VarDecl *, VariableUsage> hasIDDepVar(const Expr * Expression);
  std::pair<const FieldDecl *, VariableUsage> hasIDDepField(const Expr * Expression);
  // Stores information necessary for printing out source of error
  struct DependencyRecord {
    DependencyRecord(SourceLocation &Location, std::string &Message)
        : Location(Location), Message(Message) {}
    SourceLocation Location;
    std::string Message;
  };
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_ID_DEPENDENT_BACKWARD_BRANCH_H
