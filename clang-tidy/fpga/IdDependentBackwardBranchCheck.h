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
  // Stores information necessary for printing out source of error
  struct IDDependencyRecord {
    IDDependencyRecord(const VarDecl * Declaration, SourceLocation Location, std::string Message)
        : VariableDeclaration(Declaration), Location(Location), Message(Message) {}
    IDDependencyRecord(const FieldDecl * Declaration, SourceLocation Location, std::string Message)
        : FieldDeclaration(Declaration), Location(Location), Message(Message) {}
    IDDependencyRecord() {}
    const VarDecl * VariableDeclaration;
    const FieldDecl * FieldDeclaration;
    SourceLocation Location;
    std::string Message;
  };
  std::map<const VarDecl *, IDDependencyRecord> IDDepVarsMap;
  std::map<const FieldDecl *, IDDependencyRecord> IDDepFieldsMap;
public:
  IdDependentBackwardBranchCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void addIDDepVar(const Stmt* Statement, const VarDecl* Variable);
  void addIDDepField(const Stmt* Statement, const FieldDecl* Field);
  IDDependencyRecord * hasIDDepVar(const Expr * Expression);
  IDDependencyRecord * hasIDDepField(const Expr * Expression);
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_ID_DEPENDENT_BACKWARD_BRANCH_H
