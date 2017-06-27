//===--- PossiblyUnreachableBarrierCheck.cpp - clang-tidy------------------===//
//
// A check to ensure workgroup restrictions on barriers are obeyed
// https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/barrier.html
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PossiblyUnreachableBarrierCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace OpenCL {

void PossiblyUnreachableBarrierCheck::registerMatchers(MatchFinder *Finder) {
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

  // An OR of all the binary operators which perform an assignment
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
  const auto HAS_BAR_DESC =
	hasDescendant(
	  callExpr(callee(
	    functionDecl(anyOf(
	      hasName("barrier"),
	      hasName("work_group_barrier")
	    ))
	  )).bind("barrier")
	);
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
      forStmt(allOf(
        HAS_BAR_DESC,
        hasCondition(COND_EXPR),
	//The below will always activate, but it will bind any assignment expressions in a potential initializer
       // anyOf(
   //       hasInitializer(
     //       
       //   ),
          anything()
      //  )
      )).bind("for"),
      ifStmt(allOf(
        HAS_BAR_DESC,
        hasCondition(COND_EXPR)
      )).bind("if"),
      doStmt(allOf(
        HAS_BAR_DESC,
        hasCondition(COND_EXPR)
      )).bind("do"),
      whileStmt(allOf(
        HAS_BAR_DESC,
        hasCondition(COND_EXPR)
      )).bind("while"),
      switchStmt(allOf(
        HAS_BAR_DESC,
        hasCondition(COND_EXPR)
      )).bind("switch")
    )), this);
}

//A recrusive preorder traversal of an Expr to check if any of the DeclRefExprs that might be in it are marked as ID-dependent variables
const DeclRefExpr * PossiblyUnreachableBarrierCheck::hasIDDepDeclRef(const Expr * e) {
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

void PossiblyUnreachableBarrierCheck::check(const MatchFinder::MatchResult &Result) {
	SourceManager &ResultSM = Result.Context->getSourceManager();
	std::string TypeS;
	llvm::raw_string_ostream s(TypeS);
	PrintingPolicy Policy = Result.Context->getPrintingPolicy();
  // The first half of the callback only deals with identifying and propagating
  // ID-dependency information into the IDDepVars vector
  const auto *Variable = Result.Nodes.getNodeAs<VarDecl>("tid_dep_var");
  const auto *Statement = Result.Nodes.getNodeAs<Stmt>("straight_assignment");
  const auto *RefExpr = Result.Nodes.getNodeAs<DeclRefExpr>("assign_ref");
  const auto *PotentialVar = Result.Nodes.getNodeAs<VarDecl>("pot_tid");
  //If there's a statement we know is ID-dependent and a variable on the LHS
  if (Statement && Variable) {
  //Record that this variable is thread-dependent only if we haven't already
    if(std::find(IDDepVars.begin(), IDDepVars.end(), Variable) == IDDepVars.end()) {
      IDDepVars.push_back(Variable);
    }
  } else if (Statement) {
//	diag(Statement->getLocStart(), "assignment of unknown thread-dependent variable");
  } else if (Variable) {
//	diag(Variable->getLocStart(), "variable seems thread dependent, but can't figure out statement");
  }
  //If there is an variable references on the RHS and any variable on the LHS
  if (RefExpr && PotentialVar) {
    const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
    //If the LHS variable isn't recorded as ID-dependent but the referenced variable is
    if(std::find(IDDepVars.begin(), IDDepVars.end(), PotentialVar) == IDDepVars.end() && std::find(IDDepVars.begin(), IDDepVars.end(), RefVar) != IDDepVars.end()) {
      //Record it
      IDDepVars.push_back(PotentialVar);
    }
  }

  //We want the diagnostic to emit slightly different text so we bind on the
  // five potential conditionals (of which only one will be true) a barrier and conditional expreesion, and possibly an id call
  const auto *BarrierCall = Result.Nodes.getNodeAs<CallExpr>("barrier");
  const auto *ForAnsc = Result.Nodes.getNodeAs<ForStmt>("for");
  const auto *IfAnsc = Result.Nodes.getNodeAs<IfStmt>("if");
  const auto *DoAnsc = Result.Nodes.getNodeAs<DoStmt>("do");
  const auto *WhileAnsc = Result.Nodes.getNodeAs<WhileStmt>("while");
  const auto *SwitchAnsc = Result.Nodes.getNodeAs<SwitchStmt>("switch");
  const auto *CondExpr = Result.Nodes.getNodeAs<Expr>("cond_expr");
  const auto *IDCall = Result.Nodes.getNodeAs<CallExpr>("id_call");

  //Figure out which type of conditional we have for the select in the later diagnostics
  int type = -1;
  if (ForAnsc) {
    type = 0;
  } else if (IfAnsc) {
    type = 1;
  } else if (DoAnsc) {
    type = 2;
  } else if (WhileAnsc) {
    type = 3;
  } else if (SwitchAnsc) {
    type = 4;
  }
  if (CondExpr) {
    //If there's a get_global/local_id call
    if (IDCall) {
      //It calls one of the ID functions directly
      diag(BarrierCall->getLocStart(), "Barrier inside %select{for loop|if/else|do loop|while loop|switch}0 may not be reachable due to ID function call in condition at %1")
	<< type
	<< CondExpr->getLocStart().printToString(ResultSM);
    } else {
      //It has some DeclRefExpr(s), check for ID-dependency
      const auto * retExpr = hasIDDepDeclRef(CondExpr);
      if (retExpr) {
        //It has an ID-dependent reference
        diag(BarrierCall->getLocStart(), "Barrier inside %select{for loop|if/else|do loop|while loop|switch}0 may not be reachable due to reference to ID-dependent variable %2 in condition at %1")
		<< type
		<< CondExpr->getLocStart().printToString(ResultSM)
		<< retExpr->getDecl();
      } else {
        //Do nothing, there's nothing wrong with a non-ID-dependent conditional expression
      }
    }
  }
}

} // namespace OpenCL
} // namespace tidy
} // namespace clang
