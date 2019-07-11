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
      hasDescendant(callExpr(
        callee(functionDecl(anyOf(
          hasName("get_global_id"),
          hasName("get_local_id")
        )))//,
   //     unless(hasAncestor(
     //     arraySubscriptExpr()
       // ))
      ))
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
            hasLHS(anyOf(
	      declRefExpr(to(
                varDecl().bind("tid_dep_var")
	      )),
	      memberExpr(member(
		fieldDecl().bind("tid_dep_field")
	      ))
            ))
          ))
        )).bind("straight_assignment")
      )
    ), this);

// Bind all VarDecls that include an initializer with a variable DeclRefExpr (incase it is ID-dependent)
  Finder->addMatcher(
    stmt(forEachDescendant(
      varDecl(hasInitializer(
        forEachDescendant(stmt(anyOf(
	  declRefExpr(
            to(varDecl())//,
          //unless(hasAncestor(arraySubscriptExpr()))
          ).bind("assign_ref_var"),
	  memberExpr(
	    member(fieldDecl())
	  ).bind("assign_ref_field")
        )))
      )).bind("pot_tid_var")
    )), this);

// Bind all VarDecls that are assigned a value with a variable DeclRefExpr (in case it is ID-dependent)
  Finder->addMatcher(
    stmt(forEachDescendant(
      binaryOperator(allOf(
        ANY_ASSIGN,
        hasRHS(
          forEachDescendant(stmt(anyOf(
            declRefExpr(
              to(varDecl())//,
              //unless(hasAncestor(arraySubscriptExpr()))
            ).bind("assign_ref_var"),
	    memberExpr(
	      member(fieldDecl())
	    ).bind("assign_ref_field")
          )))
        ),
        hasLHS(anyOf(
          declRefExpr(to(
            varDecl().bind("pot_tid_var")
          )),
	  memberExpr(member(
	    fieldDecl().bind("pot_tid_field")
	  ))
        ))
      ))
    )), this);

  //Second Matcher looks for branch statements inside of loops and bind on the condition expression IF it either calls an ID function or has a variable DeclRefExpr
  //DeclRefExprs are checked later to confirm whether the variable is ID-dependent
//  const auto HAS_LOOP_ANSC = 
//    hasAncestor(stmt(anyOf(
//      forStmt().bind("loop_ansc"),
//      doStmt().bind("loop_ansc"),
//      whileStmt().bind("loop_ansc")
//    )));
  const auto COND_EXPR =
    expr(anyOf(
      hasDescendant(callExpr(
        callee(functionDecl(anyOf(
          hasName("get_global_id"),
          hasName("get_local_id")
        )))
      ).bind("id_call")),
      hasDescendant(stmt(anyOf(
	declRefExpr(
          to(varDecl())//,
        //unless(hasAncestor(arraySubscriptExpr()))
        ),
	memberExpr(
	  member(fieldDecl())
	)
      )))
    )).bind("cond_expr");
  Finder->addMatcher(
    stmt(anyOf(
//      ifStmt(allOf(
//        HAS_LOOP_ANSC,
//        hasCondition(COND_EXPR)
//      )),
//      caseStmt(allOf(
//        HAS_LOOP_ANSC,
//        hasCaseConstant(COND_EXPR)
//      )),
      forStmt(//allOf(
//        HAS_LOOP_ANSC,
        hasCondition(COND_EXPR)
      ),
      doStmt(//allOf(
//        HAS_LOOP_ANSC,
        hasCondition(COND_EXPR)
      ),
      whileStmt(//allOf(
//        HAS_LOOP_ANSC,
        hasCondition(COND_EXPR)
      )//)
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
    //if (auto * ase = dyn_cast<ArraySubscriptExpr>(e)) return NULL;
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

const MemberExpr * IdDependentBackwardBranchCheck::hasIDDepMember(const Expr * e) {
  if (const MemberExpr * expr = dyn_cast<MemberExpr>(e)) {
    //It is a DeclRefExpr, so check if it's an ID-dependent variable
    if(std::find(IDDepFields.begin(), IDDepFields.end(), dyn_cast<FieldDecl>(expr->getMemberDecl())) != IDDepFields.end()) {
      return expr;
     } else {
      return NULL;
     }
  } else {
    //If we care about thread-dependent array subscript exprs, turn off the below
    //if (auto * ase = dyn_cast<ArraySubscriptExpr>(e)) return NULL;
    //We need to iterate over its children and see if any of them are
    for (auto i = e->child_begin(); i != e->child_end(); ++i) {
      if (auto * newExpr = dyn_cast<Expr>(*i)) {
        auto * retExpr = hasIDDepMember(newExpr);
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
  const auto *Field = Result.Nodes.getNodeAs<FieldDecl>("tid_dep_field");
  const auto *Statement = Result.Nodes.getNodeAs<Stmt>("straight_assignment");
  const auto *RefExpr = Result.Nodes.getNodeAs<DeclRefExpr>("assign_ref_var");
  const auto *MemExpr = Result.Nodes.getNodeAs<MemberExpr>("assign_ref_field");
  const auto *PotentialVar = Result.Nodes.getNodeAs<VarDecl>("pot_tid_var");
  const auto *PotentialField = Result.Nodes.getNodeAs<FieldDecl>("pot_tid_field");
  if (Statement && (Variable || Field)) {
  //Record that this variable is thread-dependent
    if (Variable) {
      if(std::find(IDDepVars.begin(), IDDepVars.end(), Variable) == IDDepVars.end()) {
        IDDepVars.push_back(Variable);
      }
    diag(Statement->getBeginLoc(), "assignment of ID-dependent variable %0 declared at %1", DiagnosticIDs::Error)
	<< Variable
	<< Variable->getBeginLoc().printToString(Result.Context->getSourceManager());
    } else if (Field) {
      if(std::find(IDDepFields.begin(), IDDepFields.end(), Field) == IDDepFields.end()) {
	IDDepFields.push_back(Field);
      }
    }
  } else if (Statement) {
	diag(Statement->getBeginLoc(), "assignment of unknown thread-dependent variable or field", DiagnosticIDs::Note);
  } else if (Variable) {
	diag(Variable->getBeginLoc(), "variable seems thread dependent, but can't figure out statement", DiagnosticIDs::Note);
  } else if (Field) {
	diag(Field->getBeginLoc(), "field seems thread dependent, but can't figure out statement", DiagnosticIDs::Note);
  }
  if ((RefExpr || MemExpr) && PotentialVar) {
   if (RefExpr) {
      const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
      if(std::find(IDDepVars.begin(), IDDepVars.end(), PotentialVar) == IDDepVars.end() && std::find(IDDepVars.begin(), IDDepVars.end(), RefVar) != IDDepVars.end()) {
        IDDepVars.push_back(PotentialVar);
        diag(RefExpr->getBeginLoc(), "Inferred assignment of ID-dependent value from ID-dependent variable %0", DiagnosticIDs::Note)
	  << RefVar;
      }
    }
    if (MemExpr) {
      const auto RefField = dyn_cast<FieldDecl>(MemExpr->getMemberDecl());
      if(std::find(IDDepVars.begin(), IDDepVars.end(), PotentialVar) == IDDepVars.end() && std::find(IDDepFields.begin(), IDDepFields.end(), RefField) != IDDepFields.end()) {
        IDDepVars.push_back(PotentialVar);
        diag(MemExpr->getBeginLoc(), "Inferred assignment of ID-dependent value from ID-dependent member %0", DiagnosticIDs::Note)
	  << RefField;
      }
    }
  }
  if ((RefExpr || MemExpr) && PotentialField) {
    if (RefExpr) {
      const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
      if(std::find(IDDepFields.begin(), IDDepFields.end(), PotentialField) == IDDepFields.end() && std::find(IDDepVars.begin(), IDDepVars.end(), RefVar) != IDDepVars.end()) {
        IDDepFields.push_back(PotentialField);
        diag(RefExpr->getBeginLoc(), "Inferred assignment of ID-dependent member from ID-dependent variable %0", DiagnosticIDs::Note)
	  << RefVar;
      }
    }
    if (MemExpr) {
      const auto RefField = dyn_cast<FieldDecl>(MemExpr->getMemberDecl());
      if(std::find(IDDepFields.begin(), IDDepFields.end(), PotentialField) == IDDepFields.end() && std::find(IDDepFields.begin(), IDDepFields.end(), RefField) != IDDepFields.end()) {
        IDDepFields.push_back(PotentialField);
        diag(MemExpr->getBeginLoc(), "Inferred assignment of ID-dependent member from ID-dependent member %0", DiagnosticIDs::Note)
	  << RefField;
      }
    }
  }

  //The second part of the callback deals with checking if a branch inside a loop is thread dependent
  const auto *CondExpr = Result.Nodes.getNodeAs<Expr>("cond_expr");
  const auto *IDCall = Result.Nodes.getNodeAs<CallExpr>("id_call");
  const auto *Loop = Result.Nodes.getNodeAs<Stmt>("backward_branch");
  int loop_type = -1;
  if (Loop) {
    if (const auto DoLoop = dyn_cast<DoStmt>(Loop)) {loop_type = 0;}
    else if (const auto WhileLoop = dyn_cast<WhileStmt>(Loop)) {loop_type = 1;}
    else if (const auto ForLoop = dyn_cast<ForStmt>(Loop)) {loop_type = 2;}
  }
  if (CondExpr) {
    if (IDCall) {
      //It calls one of the ID functions directly
      diag(IDCall->getBeginLoc(), "Backward branch (%select{do|while|for}0 loop) is ID-dependent due to ID function call and may cause performance degradation")
	<< loop_type;
    } else {
      //It has some DeclRefExpr(s), check for ID-dependency
      const auto * retDeclExpr = hasIDDepDeclRef(CondExpr);
      const auto * retMemberExpr = hasIDDepMember(CondExpr);
      if (retDeclExpr) {
        //It has an ID-dependent reference
        diag(CondExpr->getBeginLoc(), "Backward branch (%select{do|while|for}0 loop) is ID-dependent due to variable reference to %1 and may cause performance degradation")
		<< loop_type
		<< retDeclExpr->getDecl();
      } else if (retMemberExpr) {
        //It has an ID-dependent reference
        diag(CondExpr->getBeginLoc(), "Backward branch (%select{do|while|for}0 loop) is ID-dependent due to member reference to %1 and may cause performance degradation")
		<< loop_type
		<< retMemberExpr->getMemberDecl();
      } else {
        //Do nothing, there's nothing wrong with a non-ID-dependent conditional expression
      }
    }
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
