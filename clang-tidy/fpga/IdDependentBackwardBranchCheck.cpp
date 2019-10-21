//===--- IdDependentBackwardBranchCheck.cpp - clang-tidy ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IdDependentBackwardBranchCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <sstream>
#include <vector>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {

void IdDependentBackwardBranchCheck::registerMatchers(MatchFinder *Finder) {
  // Prototype to identify all variables which hold a thread-variant ID
  // First Matcher just finds all the direct assignments of either ID call
  const auto TID_RHS = expr(hasDescendant(callExpr(callee(functionDecl(
      anyOf(hasName("get_global_id"), hasName("get_local_id")))))));

  const auto ANY_ASSIGN = anyOf(
      hasOperatorName("="), hasOperatorName("*="), hasOperatorName("/="),
      hasOperatorName("%="), hasOperatorName("+="), hasOperatorName("-="),
      hasOperatorName("<<="), hasOperatorName(">>="), hasOperatorName("&="),
      hasOperatorName("^="), hasOperatorName("|="));

  Finder->addMatcher(
      compoundStmt(
          // Bind on actual get_local/global_id calls
          forEachDescendant(
              stmt(anyOf(declStmt(hasDescendant(varDecl(hasInitializer(TID_RHS))
                                                    .bind("tid_dep_var"))),
                         binaryOperator(allOf(
                             ANY_ASSIGN, hasRHS(TID_RHS),
                             hasLHS(anyOf(
                                 declRefExpr(to(varDecl().bind("tid_dep_var"))),
                                 memberExpr(member(
                                     fieldDecl().bind("tid_dep_field")))))))))
                  .bind("straight_assignment"))),
      this);

  // Bind all VarDecls that include an initializer with a variable DeclRefExpr
  // (incase it is ID-dependent)
  Finder->addMatcher(
      stmt(forEachDescendant(
          varDecl(
              hasInitializer(forEachDescendant(stmt(anyOf(
                  declRefExpr(to(varDecl())).bind("assign_ref_var"),
                  memberExpr(member(fieldDecl())).bind("assign_ref_field"))))))
              .bind("pot_tid_var"))),
      this);

  // Bind all VarDecls that are assigned a value with a variable DeclRefExpr (in
  // case it is ID-dependent)
  Finder->addMatcher(
      stmt(forEachDescendant(binaryOperator(allOf(
          ANY_ASSIGN,
          hasRHS(forEachDescendant(stmt(anyOf(
              declRefExpr(to(varDecl())).bind("assign_ref_var"),
              memberExpr(member(fieldDecl())).bind("assign_ref_field"))))),
          hasLHS(
              anyOf(declRefExpr(to(varDecl().bind("pot_tid_var"))),
                    memberExpr(member(fieldDecl().bind("pot_tid_field"))))))))),
      this);

  // Second Matcher looks for branch statements inside of loops and bind on the
  // condition expression IF it either calls an ID function or has a variable
  // DeclRefExpr DeclRefExprs are checked later to confirm whether the variable
  // is ID-dependent
  const auto COND_EXPR =
      expr(anyOf(hasDescendant(callExpr(callee(functionDecl(
                                            anyOf(hasName("get_global_id"),
                                                  hasName("get_local_id")))))
                                   .bind("id_call")),
                 hasDescendant(stmt(anyOf(declRefExpr(to(varDecl())),
                                          memberExpr(member(fieldDecl())))))))
          .bind("cond_expr");
  Finder->addMatcher(stmt(anyOf(forStmt(hasCondition(COND_EXPR)),
                                doStmt(hasCondition(COND_EXPR)),
                                whileStmt(hasCondition(COND_EXPR))))
                         .bind("backward_branch"),
                     this);
}

IdDependentBackwardBranchCheck::IDDependencyRecord *
IdDependentBackwardBranchCheck::hasIDDepVar(const Expr *Expression) {
  if (const DeclRefExpr *expr = dyn_cast<DeclRefExpr>(Expression)) {
    // It is a DeclRefExpr, so check if it's an ID-dependent variable
    const VarDecl *CheckVariable = dyn_cast<VarDecl>(expr->getDecl());
    auto FoundVariable = IDDepVarsMap.find(CheckVariable);
    if (FoundVariable == IDDepVarsMap.end()) {
      return nullptr;
    }
    return &(FoundVariable->second);
  }
  for (auto i = Expression->child_begin(), e = Expression->child_end(); i != e;
       ++i) {
    if (auto *ChildExpression = dyn_cast<Expr>(*i)) {
      auto Result = hasIDDepVar(ChildExpression);
      if (Result) {
        return Result;
      }
    }
  }
  return nullptr;
}

IdDependentBackwardBranchCheck::IDDependencyRecord *
IdDependentBackwardBranchCheck::hasIDDepField(const Expr *Expression) {
  if (const MemberExpr *MemberExpression = dyn_cast<MemberExpr>(Expression)) {
    const FieldDecl *CheckField =
        dyn_cast<FieldDecl>(MemberExpression->getMemberDecl());
    auto FoundField = IDDepFieldsMap.find(CheckField);
    if (FoundField == IDDepFieldsMap.end()) {
      return nullptr;
    }
    return &(FoundField->second);
  }
  for (auto I = Expression->child_begin(), E = Expression->child_end(); I != E;
       ++I) {
    if (auto *ChildExpression = dyn_cast<Expr>(*I)) {
      auto Result = hasIDDepField(ChildExpression);
      if (Result) {
        return Result;
      }
    }
  }
  return nullptr;
}

void IdDependentBackwardBranchCheck::saveIDDepVar(const Stmt *Statement,
                                                  const VarDecl *Variable) {
  // Record that this variable is thread-dependent
  std::ostringstream StringStream;
  StringStream << "assignment of ID-dependent variable "
               << Variable->getNameAsString();
  IDDepVarsMap[Variable] =
      IDDependencyRecord(Variable, Variable->getBeginLoc(), StringStream.str());
}

void IdDependentBackwardBranchCheck::saveIDDepField(const Stmt *Statement,
                                                    const FieldDecl *Field) {
  std::ostringstream StringStream;
  StringStream << "assignment of ID-dependent field "
               << Field->getNameAsString();
  IDDepFieldsMap[Field] =
      IDDependencyRecord(Field, Statement->getBeginLoc(), StringStream.str());
}

void IdDependentBackwardBranchCheck::saveIDDepVarFromReference(
    const DeclRefExpr *RefExpr, const MemberExpr *MemExpr,
    const VarDecl *PotentialVar) {
  // If the variable is already in IDDepVarsMap, ignore it
  if (IDDepVarsMap.find(PotentialVar) != IDDepVarsMap.end()) {
    return;
  }
  std::ostringstream StringStream;
  StringStream << "inferred assignment of ID-dependent value from "
                  "ID-dependent ";
  if (RefExpr) {
    const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
    // If variable isn't ID-dependent, but refVar is
    if (IDDepVarsMap.find(RefVar) != IDDepVarsMap.end()) {
      StringStream << "variable " << RefVar->getNameAsString();
    }
  }
  if (MemExpr) {
    const auto RefField = dyn_cast<FieldDecl>(MemExpr->getMemberDecl());
    // If variable isn't ID-dependent, but refField is
    if (IDDepFieldsMap.find(RefField) != IDDepFieldsMap.end()) {
      StringStream << "member " << RefField->getNameAsString();
    }
  }
  IDDepVarsMap[PotentialVar] = IDDependencyRecord(
      PotentialVar, PotentialVar->getBeginLoc(), StringStream.str());
}

void IdDependentBackwardBranchCheck::saveIDDepFieldFromReference(
    const DeclRefExpr *RefExpr, const MemberExpr *MemExpr,
    const FieldDecl *PotentialField) {
  // If the field is already in IDDepFieldsMap, ignore it
  if (IDDepFieldsMap.find(PotentialField) != IDDepFieldsMap.end()) {
    return;
  }
  std::ostringstream StringStream;
  StringStream << "inferred assignment of ID-dependent member from "
                  "ID-dependent ";
  if (RefExpr) {
    const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
    // If field isn't ID-dependent, but RefVar is
    if (IDDepVarsMap.find(RefVar) != IDDepVarsMap.end()) {
      StringStream << "variable " << RefVar->getNameAsString();
    }
  }
  if (MemExpr) {
    const auto RefField = dyn_cast<FieldDecl>(MemExpr->getMemberDecl());
    if (IDDepFieldsMap.find(RefField) != IDDepFieldsMap.end()) {
      StringStream << "member " << RefField->getNameAsString();
    }
  }
  IDDepFieldsMap[PotentialField] = IDDependencyRecord(
      PotentialField, PotentialField->getBeginLoc(), StringStream.str());
}

IdDependentBackwardBranchCheck::LoopType
IdDependentBackwardBranchCheck::getLoopType(const Stmt *Loop) {
  if (const auto DoLoop = dyn_cast<DoStmt>(Loop)) {
    return DO_LOOP; // loop_type = 0;
  } else if (const auto WhileLoop = dyn_cast<WhileStmt>(Loop)) {
    return WHILE_LOOP; // loop_type = 1;
  } else if (const auto ForLoop = dyn_cast<ForStmt>(Loop)) {
    return FOR_LOOP; // loop_type = 2;
  }
  return UNK_LOOP;
}

void IdDependentBackwardBranchCheck::check(
    const MatchFinder::MatchResult &Result) {
  // The first half of the callback only deals with identifying and propagating
  // ID-dependency information into the IDDepVars vector
  const auto *Variable = Result.Nodes.getNodeAs<VarDecl>("tid_dep_var");
  const auto *Field = Result.Nodes.getNodeAs<FieldDecl>("tid_dep_field");
  const auto *Statement = Result.Nodes.getNodeAs<Stmt>("straight_assignment");
  const auto *RefExpr = Result.Nodes.getNodeAs<DeclRefExpr>("assign_ref_var");
  const auto *MemExpr = Result.Nodes.getNodeAs<MemberExpr>("assign_ref_field");
  const auto *PotentialVar = Result.Nodes.getNodeAs<VarDecl>("pot_tid_var");
  const auto *PotentialField =
      Result.Nodes.getNodeAs<FieldDecl>("pot_tid_field");

  // Add variables and fields assigned directly through ID function calls
  if (Statement && (Variable || Field)) {
    if (Variable) {
      saveIDDepVar(Statement, Variable);
    } else if (Field) {
      saveIDDepField(Statement, Field);
    }
  }

  // Add variables assigned to values of Id-dependent variables and fields
  if ((RefExpr || MemExpr) && PotentialVar) {
    saveIDDepVarFromReference(RefExpr, MemExpr, PotentialVar);
  }

  // Add fields assigned to values of ID-dependent variables and fields
  if ((RefExpr || MemExpr) && PotentialField) {
    saveIDDepFieldFromReference(RefExpr, MemExpr, PotentialField);
  }

  // The second part of the callback deals with checking if a branch inside a
  // loop is thread dependent
  const auto *CondExpr = Result.Nodes.getNodeAs<Expr>("cond_expr");
  const auto *IDCall = Result.Nodes.getNodeAs<CallExpr>("id_call");
  const auto *Loop = Result.Nodes.getNodeAs<Stmt>("backward_branch");
  if (!Loop) {
    return;
  }
  LoopType Type = getLoopType(Loop);
  if (CondExpr) {
    if (IDCall) {
      // It calls one of the ID functions directly
      diag(CondExpr->getBeginLoc(),
           "backward branch (%select{do|while|for}0 loop) is ID-dependent due "
           "to ID function call and may cause performance degradation")
          << Type;
    } else {
      // It has some DeclRefExpr(s), check for ID-dependency
      // VariableUsage is a Vector of SourceLoc, string pairs
      IDDependencyRecord *IDDepVar = hasIDDepVar(CondExpr);
      IDDependencyRecord *IDDepField = hasIDDepField(CondExpr);
      if (IDDepVar) {
        diag(IDDepVar->Location, IDDepVar->Message);
        diag(
            CondExpr->getBeginLoc(),
            "backward branch (%select{do|while|for}0 loop) is ID-dependent due "
            "to variable reference to %1 and may cause performance degradation")
            << Type << IDDepVar->VariableDeclaration;
      } else if (IDDepField) {
        diag(IDDepField->Location, IDDepField->Message);
        diag(
            CondExpr->getBeginLoc(),
            "backward branch (%select{do|while|for}0 loop) is ID-dependent due "
            "to member reference to %1 and may cause performance degradation")
            << Type << IDDepField->FieldDeclaration;
      }
    }
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
