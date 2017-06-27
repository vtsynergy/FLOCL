//===--- IdDependentBackwardBranchCheck.cpp - clang-tidy-------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>
#include "IdDependentBackwardBranchCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace FPGA {


void IdDependentBackwardBranchCheck::registerMatchers(MatchFinder *Finder) {
  //Prototype to identify all variables which hold a thread-variant ID
  // First Matcher just finds all the direct assignments of either ID call
  const auto TID_RHS =
    expr(
      hasDescendant(callExpr(allOf(
        callee(functionDecl(anyOf(
          hasName("get_global_id"),
          hasName("get_local_id")
        ))),
        unless(hasAncestor(
          arraySubscriptExpr()
        ))
      )))
    );

  const auto ANY_ASSIGN =
    anyOf(
      hasOperatorName("="),
      hasOperatorName("*="),
      hasOperatorName("/="),
      hasOperatorName("%="),
      hasOperatorName("+="),
      hasOperatorName("-="),
      hasOperatorName("<<="),
      hasOperatorName(">>="),
      hasOperatorName("&="),
      hasOperatorName("^="),
      hasOperatorName("|=") 
    );

  Finder->addMatcher(
    compoundStmt(
      //Bind on actual get_local/global_id calls
      forEachDescendant(
        stmt(anyOf(
          declStmt(hasDescendant(
            varDecl(hasInitializer(TID_RHS)).bind("tid_dep_var")
          )),
          binaryOperator(allOf(
            ANY_ASSIGN,
            hasRHS(TID_RHS),
            hasLHS(declRefExpr(to(
              varDecl().bind("tid_dep_var")
            )))
          ))
        )).bind("straight_assignment")
      )
    ), this);

// Bind all VarDecls that include an initializer with a variable DeclRefExpr (incase it is ID-dependent)
  Finder->addMatcher(
    stmt(forEachDescendant(
      varDecl(hasInitializer(
        forEachDescendant(declRefExpr(
          to(varDecl()),
          unless(hasAncestor(arraySubscriptExpr()))
        ).bind("assign_ref"))
      )).bind("pot_tid")
    )), this);

// Bind all VarDecls that are assigned a value with a variable DeclRefExpr (in case it is ID-dependent)
  Finder->addMatcher(
    stmt(forEachDescendant(
      binaryOperator(allOf(
        ANY_ASSIGN,
        hasRHS(
          forEachDescendant(declRefExpr(
            to(varDecl()),
            unless(hasAncestor(arraySubscriptExpr()))
          ).bind("assign_ref"))
        ),
        hasLHS(declRefExpr(to(
          varDecl().bind("pot_tid")
        )))
      ))
    )), this);

  //Second Matcher looks for branch statements inside of loops and bind on the condition expression IF it either calls an ID function or has a variable DeclRefExpr
  //DeclRefExprs are checked later to confirm whether the variable is ID-dependent
  const auto HAS_LOOP_ANSC = 
    hasAncestor(stmt(anyOf(
      forStmt().bind("loop_ansc"),
      doStmt().bind("loop_ansc"),
      whileStmt().bind("loop_ansc")
    )));
  const auto COND_EXPR =
    expr(anyOf(
      hasDescendant(callExpr(
        callee(functionDecl(anyOf(
          hasName("get_global_id"),
          hasName("get_local_id")
        )))
      ).bind("id_call")),
      hasDescendant(declRefExpr(
        to(varDecl()),
        unless(hasAncestor(arraySubscriptExpr()))
      ))
    )).bind("cond_expr");
  Finder->addMatcher(
    stmt(anyOf(
      ifStmt(allOf(
        HAS_LOOP_ANSC,
        hasCondition(COND_EXPR)
      )),
      caseStmt(allOf(
        HAS_LOOP_ANSC,
        hasCaseConstant(COND_EXPR)
      )),
      forStmt(allOf(
        HAS_LOOP_ANSC,
        hasCondition(COND_EXPR)
      )),
      doStmt(allOf(
        HAS_LOOP_ANSC,
        hasCondition(COND_EXPR)
      )),
      whileStmt(allOf(
        HAS_LOOP_ANSC,
        hasCondition(COND_EXPR)
      ))
    )).bind("backward_branch"), this);
}

const DeclRefExpr * IdDependentBackwardBranchCheck::hasIDDepDeclRef(const Expr * e) {
  if (const DeclRefExpr * expr = dyn_cast<DeclRefExpr>(e)) {
    //It is a DeclRefExpr, so check if it's an ID-dependent variable
    if(std::find(IDDepVars.begin(), IDDepVars.end(), dyn_cast<VarDecl>(expr->getDecl())) != IDDepVars.end()) {
      return expr;
     } else {
      return NULL;
     }
  } else {
    //If we care about thread-dependent array subscript exprs, turn off the below
    if (auto * ase = dyn_cast<ArraySubscriptExpr>(e)) return NULL;
    //We need to iterate over its children and see if any of them are
    for (auto i = e->child_begin(); i != e->child_end(); ++i) {
      if (auto * newExpr = dyn_cast<Expr>(*i)) {
        auto * retExpr = hasIDDepDeclRef(newExpr);
        if (retExpr) return retExpr;
      }
    }
    //If none of the children force an early return with a match, return NULL
    return NULL;
  }
  return NULL; //Should be unreachable
}

void IdDependentBackwardBranchCheck::check(const MatchFinder::MatchResult &Result) {
  // The first half of the callback only deals with identifying and propagating
  // ID-dependency information into the IDDepVars vector
  const auto *Variable = Result.Nodes.getNodeAs<VarDecl>("tid_dep_var");
  const auto *Statement = Result.Nodes.getNodeAs<Stmt>("straight_assignment");
  const auto *RefExpr = Result.Nodes.getNodeAs<DeclRefExpr>("assign_ref");
  const auto *PotentialVar = Result.Nodes.getNodeAs<VarDecl>("pot_tid");
  if (Statement && Variable) {
  //Record that this variable is thread-dependent
    if(std::find(IDDepVars.begin(), IDDepVars.end(), Variable) == IDDepVars.end()) {
      IDDepVars.push_back(Variable);
    }
  diag(Statement->getLocStart(), "assignment of ID-dependent variable %0 declared at %1")
	<< Variable
	<< Variable->getLocStart().printToString(Result.Context->getSourceManager());
  } else if (Statement) {
	diag(Statement->getLocStart(), "assignment of unknown thread-dependent variable");
  } else if (Variable) {
	diag(Variable->getLocStart(), "variable seems thread dependent, but can't figure out statement");
  }
  if (RefExpr && PotentialVar) {
    const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
    if(std::find(IDDepVars.begin(), IDDepVars.end(), PotentialVar) == IDDepVars.end() && std::find(IDDepVars.begin(), IDDepVars.end(), RefVar) != IDDepVars.end()) {
      IDDepVars.push_back(PotentialVar);
      diag(RefExpr->getLocStart(), "Inferred assignment of ID-dependent value from ID-dependent variable %0")
	<< RefVar;
    }
  }

  //The second part of the callback deals with checking if a branch inside a loop is thread dependent
  const auto *CondExpr = Result.Nodes.getNodeAs<Expr>("cond_expr");
  const auto *IDCall = Result.Nodes.getNodeAs<CallExpr>("id_call");
  if (CondExpr) {
    if (IDCall) {
      //It calls one of the ID functions directly
      diag(IDCall->getLocStart(), "Conditional inside loop is ID-dependent due to ID function call");
    } else {
      //It has some DeclRefExpr(s), check for ID-dependency
      const auto * retExpr = hasIDDepDeclRef(CondExpr);
      if (retExpr) {
        //It has an ID-dependent reference
        diag(CondExpr->getLocStart(), "Conditional inside loop is ID-dependent due to variable reference to %0")
		<< retExpr->getDecl();
      } else {
        //Do nothing, there's nothing wrong with a non-ID-dependent conditional expression
      }
    }
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
