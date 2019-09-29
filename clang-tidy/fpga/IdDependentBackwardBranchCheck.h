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

/// Finds ID-dependent variables and fields used within loops, and warns of
/// their usage. Using these variables in loops can lead to performance
/// degradation.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/FPGA-ID-dependent-backward-branch.html
class IdDependentBackwardBranchCheck : public ClangTidyCheck {
private:
  enum LoopType { UNK_LOOP = -1, DO_LOOP = 0, WHILE_LOOP = 1, FOR_LOOP = 2 };
  // Stores information necessary for printing out source of error.
  struct IDDependencyRecord {
    IDDependencyRecord(const VarDecl *Declaration, SourceLocation Location,
                       std::string Message)
        : VariableDeclaration(Declaration), Location(Location),
          Message(Message) {}
    IDDependencyRecord(const FieldDecl *Declaration, SourceLocation Location,
                       std::string Message)
        : FieldDeclaration(Declaration), Location(Location), Message(Message) {}
    IDDependencyRecord() {}
    const VarDecl *VariableDeclaration;
    const FieldDecl *FieldDeclaration;
    SourceLocation Location;
    std::string Message;
  };
  // Stores the locations where ID-dependent variables are created.
  std::map<const VarDecl *, IDDependencyRecord> IDDepVarsMap;
  // Stores the locations where ID-dependent fields are created.
  std::map<const FieldDecl *, IDDependencyRecord> IDDepFieldsMap;
  /// Returns an IDDependencyRecord if the Expression contains an ID-dependent
  /// variable, returns a nullptr otherwise.
  IDDependencyRecord *hasIDDepVar(const Expr *Expression);
  /// Returns an IDDependencyRecord if the Expression contains an ID-dependent
  /// field, returns a nullptr otherwise.
  IDDependencyRecord *hasIDDepField(const Expr *Expression);
  /// Stores the location an ID-dependent variable is created from a call to
  /// an ID function in IDDepVarsMap.
  void saveIDDepVar(const Stmt *Statement, const VarDecl *Variable);
  /// Stores the location an ID-dependent field is created from a call to an ID
  /// function in IDDepFieldsMap.
  void saveIDDepField(const Stmt *Statement, const FieldDecl *Field);
  /// Stores the location an ID-dependent variable is created from a reference
  /// to another ID-dependent variable or field in IDDepVarsMap.
  void saveIDDepVarFromReference(const DeclRefExpr *RefExpr,
                                 const MemberExpr *MemExpr,
                                 const VarDecl *PotentialVar);
  /// Stores the location an ID-dependent field is created from a reference to
  /// another ID-dependent variable or field in IDDepFieldsMap.
  void saveIDDepFieldFromReference(const DeclRefExpr *RefExpr,
                                   const MemberExpr *MemExpr,
                                   const FieldDecl *PotentialField);
  /// Returns the loop type.
  LoopType getLoopType(const Stmt *Loop);

public:
  IdDependentBackwardBranchCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace FPGA
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FPGA_ID_DEPENDENT_BACKWARD_BRANCH_H
