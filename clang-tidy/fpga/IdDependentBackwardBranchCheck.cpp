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
	      declRefExpr(to(varDecl())).bind("assign_ref_var"),
	      memberExpr(member(fieldDecl())).bind("assign_ref_field")
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

  // Second Matcher looks for branch statements inside of loops and bind on the condition expression IF it either calls an ID function or has a variable DeclRefExpr
  // DeclRefExprs are checked later to confirm whether the variable is ID-dependent
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
        ),
	memberExpr(
	  member(fieldDecl())
	)
      )))
    )).bind("cond_expr");
  Finder->addMatcher(
    stmt(anyOf(
      forStmt(//allOf(
        hasCondition(COND_EXPR)
      ),
      doStmt(//allOf(
        hasCondition(COND_EXPR)
      ),
      whileStmt(//allOf(
        hasCondition(COND_EXPR)
      )//)
    )).bind("backward_branch"), this);
}

std::pair<const VarDecl *, VariableUsage> 
IdDependentBackwardBranchCheck::hasIDDepVar(const Expr * Expression) {
  if (const DeclRefExpr * expr = dyn_cast<DeclRefExpr>(Expression)) {
    // It is a DeclRefExpr, so check if it's an ID-dependent variable
    const VarDecl * CheckVariable = dyn_cast<VarDecl>(expr->getDecl());
    auto FoundVariable = IDDepVarsMap.find(CheckVariable);
    if (FoundVariable == IDDepVarsMap.end()) {
      return std::make_pair(nullptr, std::vector<std::pair<SourceLocation, std::string>>());
    }
    return std::make_pair(FoundVariable->first, FoundVariable->second);
  }
  for (auto i = Expression->child_begin(), e = Expression->child_end(); i != e; ++i) {
    if (auto * ChildExpression = dyn_cast<Expr>(*i)) {
      auto Result = hasIDDepVar(ChildExpression);
      if (Result.first) {
        return Result;
      }
    }
  } 
  return std::make_pair(nullptr, std::vector<std::pair<SourceLocation, std::string>>());
}

std::pair<const FieldDecl *, VariableUsage>
IdDependentBackwardBranchCheck::hasIDDepField(const Expr * Expression) {
  if (const MemberExpr * MemberExpression = dyn_cast<MemberExpr>(Expression)) {
    const FieldDecl * CheckField = dyn_cast<FieldDecl>(MemberExpression->getMemberDecl());
    auto FoundField = IDDepFieldsMap.find(CheckField);
    if (FoundField == IDDepFieldsMap.end()) {
      return std::make_pair(nullptr, std::vector<std::pair<SourceLocation, std::string>>());
    }
    return std::make_pair(FoundField->first, FoundField->second);
  }
  for (auto I = Expression->child_begin(), E = Expression->child_end(); I != E; ++I) {
    if (auto * ChildExpression = dyn_cast<Expr>(*I)) {
      auto Result = hasIDDepField(ChildExpression);
      if (Result.first) {
        return Result;
      }
    }
  }
  return std::make_pair(nullptr, std::vector<std::pair<SourceLocation, std::string>>());
}

void IdDependentBackwardBranchCheck::addIDDepVar(const Stmt* Statement, const VarDecl* Variable) {
  //Record that this variable is thread-dependent
  std::ostringstream StringStream;
  StringStream << "assignment of ID-dependent variable " << Variable->getNameAsString(); //  << " declared at "
  auto FoundVariable = IDDepVarsMap.find(Variable);
  if (FoundVariable == IDDepVarsMap.end()) {  // Put empty list if there isn't one already
    IDDepVarsMap[Variable] = std::vector<std::pair<SourceLocation, std::string>>();
  }
  IDDepVarsMap[Variable].emplace_back(Variable->getBeginLoc(), StringStream.str());
}

void IdDependentBackwardBranchCheck::addIDDepField(const Stmt* Statement, const FieldDecl* Field) {
  std::ostringstream StringStream;
  StringStream << "assignment of ID-dependent field " << Field->getNameAsString(); //  << " declared at "
  auto FoundField = IDDepFieldsMap.find(Field);
  if (FoundField == IDDepFieldsMap.end()) {  // Put empty list if there isn't one already
    // diag(Statement->getBeginLoc(), "Identified IDDep field %0 for the first time") << Field;
    IDDepFieldsMap[Field] = std::vector<std::pair<SourceLocation, std::string>>();
  }
  IDDepFieldsMap[Field].emplace_back(Statement->getBeginLoc(), StringStream.str());
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
 
  // Add variables and fields assigned directly through ID function calls 
  if (Statement && (Variable || Field)) {
    if (Variable) {
        addIDDepVar(Statement, Variable);
    } else if (Field) {
        addIDDepField(Statement, Field);
    }
  }

  // Add variables assigned to values of Id-dependent variables and fields
  if ((RefExpr || MemExpr) && PotentialVar) {
   auto FoundPotentialVar = IDDepVarsMap.find(PotentialVar);
   if (RefExpr) {
      const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
      auto FoundRefVar = IDDepVarsMap.find(RefVar);
      // If variable isn't ID-dependent, but refVar is
      if (FoundPotentialVar == IDDepVarsMap.end() && FoundRefVar != IDDepVarsMap.end()) {
        std::ostringstream StringStream;
        StringStream << "inferred assignment of ID-dependent value from ID-dependent variable '";
        StringStream << RefVar->getNameAsString() << "'"; 
        IDDepVarsMap[PotentialVar].emplace_back(PotentialVar->getBeginLoc(), StringStream.str());
      }
    }
    if (MemExpr) {
      const auto RefField = dyn_cast<FieldDecl>(MemExpr->getMemberDecl());
      auto FoundRefField = IDDepFieldsMap.find(RefField);
      // If variable isn't ID-dependent, but refField is
      if (FoundPotentialVar == IDDepVarsMap.end() && FoundRefField != IDDepFieldsMap.end()) {
        std::ostringstream StringStream;
        StringStream << "inferred assignment of ID-dependent value from ID-dependent member '";
        StringStream << RefField->getNameAsString() << "'";
        IDDepVarsMap[PotentialVar].emplace_back(PotentialVar->getBeginLoc(), StringStream.str());
      }
    }
  }
  
  // Add fields assigned to values of ID-dependent variables and fields
  if ((RefExpr || MemExpr) && PotentialField) {
    auto FoundPotentialField = IDDepFieldsMap.find(PotentialField);
    if (RefExpr) {
      const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
      auto FoundRefVar = IDDepVarsMap.find(RefVar);
      // If field isn't ID-dependent, but RefVar is
      if (FoundPotentialField == IDDepFieldsMap.end() && FoundRefVar != IDDepVarsMap.end()) {
        std::ostringstream StringStream;
        StringStream << "inferred assignment of ID-dependent member from ID-dependent variable '";
        StringStream << RefVar->getNameAsString() << "'";
        IDDepFieldsMap[PotentialField].emplace_back(PotentialField->getBeginLoc(), StringStream.str());
        // IDDepFields.push_back(PotentialField);
        // diag(RefExpr->getBeginLoc(), "Inferred assignment of ID-dependent member from ID-dependent variable %0", DiagnosticIDs::Note)
	    // << RefVar;
      }
    }
    if (MemExpr) {
      const auto RefField = dyn_cast<FieldDecl>(MemExpr->getMemberDecl());
      auto FoundRefField = IDDepFieldsMap.find(RefField);
      if (FoundPotentialField == IDDepFieldsMap.end() && FoundRefField != IDDepFieldsMap.end()) {
          // std::find(IDDepFields.begin(), IDDepFields.end(), PotentialField) == IDDepFields.end() && std::find(IDDepFields.begin(), IDDepFields.end(), RefField) != IDDepFields.end()) {
          std::ostringstream StringStream;
          StringStream << "inferred assignment of ID-dependent member from ID-dependent member '";
          StringStream << RefField->getNameAsString() << "'";
          IDDepFieldsMap[PotentialField].emplace_back(PotentialField->getBeginLoc(), StringStream.str());
          // IDDepFields.push_back(PotentialField);
        // diag(MemExpr->getBeginLoc(), "Inferred assignment of ID-dependent member from ID-dependent member %0", DiagnosticIDs::Note)
	    // << RefField;
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
      diag(CondExpr->getBeginLoc(), "Backward branch (%select{do|while|for}0 loop) is ID-dependent due to ID function call and may cause performance degradation")
	<< loop_type;
    } else {
      // It has some DeclRefExpr(s), check for ID-dependency
      // VariableUsage is a Vector of SourceLoc, string pairs
      std::pair<const VarDecl *, VariableUsage> IDDepVar = hasIDDepVar(CondExpr);
      std::pair<const FieldDecl *, VariableUsage> IDDepField = hasIDDepField(CondExpr);
      if (IDDepVar.first) {
        for (std::pair<SourceLocation, std::string> Traceback : IDDepVar.second) {
          diag(Traceback.first, Traceback.second);
        }
        // It has an ID-dependent reference
        diag(CondExpr->getBeginLoc(), "Backward branch (%select{do|while|for}0 loop) is ID-dependent due to variable reference to %1 and may cause performance degradation")
		<< loop_type
        << IDDepVar.first;
      } else if (IDDepField.first) {
        for (std::pair<SourceLocation, std::string> Traceback : IDDepField.second) {
          diag(Traceback.first, Traceback.second);
        }
        // It has an ID-dependent reference
        diag(CondExpr->getBeginLoc(), "Backward branch (%select{do|while|for}0 loop) is ID-dependent due to member reference to %1 and may cause performance degradation")
		<< loop_type
        << IDDepField.first;
      }
    }
  }
}

} // namespace FPGA
} // namespace tidy
} // namespace clang
