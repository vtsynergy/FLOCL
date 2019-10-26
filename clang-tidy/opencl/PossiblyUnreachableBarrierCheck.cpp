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

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace OpenCL {

void PossiblyUnreachableBarrierCheck::registerMatchers(MatchFinder *Finder) {
  // Prototype to identify all variables which hold a thread-variant ID
  // First Matcher just finds all the direct assignments of either ID call
  const auto TID_RHS = expr(hasDescendant(callExpr(callee(functionDecl(
      anyOf(hasName("get_global_id"), hasName("get_local_id")))))));

  // An OR of all the binary operators which perform an assignment
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
  const auto HAS_BAR_DESC = hasDescendant(
      callExpr(callee(functionDecl(
                   anyOf(hasName("barrier"), hasName("work_group_barrier")))))
          .bind("barrier"));
  const auto COND_EXPR =
      expr(anyOf(hasDescendant(callExpr(callee(functionDecl(
                                            anyOf(hasName("get_global_id"),
                                                  hasName("get_local_id")))))
                                   .bind("id_call")),
                 hasDescendant(stmt(anyOf(declRefExpr(to(varDecl())),
                                          memberExpr(member(fieldDecl())))))))
          .bind("cond_expr");

  Finder->addMatcher(
      stmt(anyOf(
          forStmt(allOf(HAS_BAR_DESC, hasCondition(COND_EXPR),
                        // The below will always activate, but it will bind any
                        // assignment expressions in a potential initializer
                        // anyOf(
                        //       hasInitializer(
                        //
                        //   ),
                        anything()
                        //  )
                        ))
              .bind("for"),
          // TODO, needs to not select elseifs, just the first if in a
          // if/elseif/else tree
          // TODO: I think I am going to implement that at the callback stage
          ifStmt(allOf(HAS_BAR_DESC,
                       hasCondition(COND_EXPR) //,
                       //	unless(hasParent(ifStmt(hasElse(this))))
                       ))
              .bind("if"),
          doStmt(allOf(HAS_BAR_DESC, hasCondition(COND_EXPR))).bind("do"),
          whileStmt(allOf(HAS_BAR_DESC, hasCondition(COND_EXPR))).bind("while"),
          switchStmt(allOf(HAS_BAR_DESC, hasCondition(COND_EXPR)))
              .bind("switch"))),
      this);
}

void PossiblyUnreachableBarrierCheck::findIDDependentVariablesAndFields(
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
  // If there's a statement we know is ID-dependent and a variable on the LHS
  if (Statement && (Variable || Field)) {
    // Record that this variable is thread-dependent only if we haven't already
    if (Variable) {
      if (std::find(IDDepVars.begin(), IDDepVars.end(), Variable) ==
          IDDepVars.end()) {
        IDDepVars.push_back(Variable);
      }
    } else if (Field) {
      if (std::find(IDDepFields.begin(), IDDepFields.end(), Field) ==
          IDDepFields.end()) {
        IDDepFields.push_back(Field);
      }
    }
  }
  // If there is an variable references on the RHS and any variable on the LHS
  if ((RefExpr || MemExpr) && PotentialVar) {
    // If PotentialVar is already in list of ID-dependent variables, return
    if (std::find(IDDepVars.begin(), IDDepVars.end(), PotentialVar) !=
            IDDepVars.end())
      return;
    
    if (RefExpr) {
      const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
      // If RefVar is in IDDepVars, PotentialVar is ID-dependent
      if (std::find(IDDepVars.begin(), IDDepVars.end(), RefVar) !=
              IDDepVars.end()) {
        IDDepVars.push_back(PotentialVar);
      }
    }
    if (MemExpr) {
      const auto RefField = dyn_cast<FieldDecl>(MemExpr->getMemberDecl());
      // If RefField is in IDDepFields, PotentialVar is ID-dependent
      if (std::find(IDDepFields.begin(), IDDepFields.end(), RefField) !=
              IDDepFields.end()) {
        IDDepVars.push_back(PotentialVar);
      }
    }
  }
  if ((RefExpr || MemExpr) && PotentialField) {
    // If PotentialField is already in list of ID-dependent fields, return
    if (std::find(IDDepFields.begin(), IDDepFields.end(), PotentialField) != 
            IDDepFields.end())
      return;

    if (RefExpr) {
      const auto RefVar = dyn_cast<VarDecl>(RefExpr->getDecl());
      // If RefVar is in IDDepVars, PotentialField is ID-dependent
      if (std::find(IDDepVars.begin(), IDDepVars.end(), RefVar) !=
              IDDepVars.end()) {
        IDDepFields.push_back(PotentialField);
      }
    }
    if (MemExpr) {
      const auto RefField = dyn_cast<FieldDecl>(MemExpr->getMemberDecl());
      // If RefField is in IDDepFields, PotentialField is ID-dependent
      if (std::find(IDDepFields.begin(), IDDepFields.end(), RefField) !=
              IDDepFields.end()) {
        IDDepFields.push_back(PotentialField);
      }
    }
  }
}

// A recrusive preorder traversal of an Expr to check if any of the DeclRefExprs
// that might be in it are marked as ID-dependent variables
const DeclRefExpr *
PossiblyUnreachableBarrierCheck::hasIDDepDeclRef(const Expr *e) {
  if (const DeclRefExpr *expr = dyn_cast<DeclRefExpr>(e)) {
    // It is a DeclRefExpr, so check if it's an ID-dependent variable
    if (std::find(IDDepVars.begin(), IDDepVars.end(),
                  dyn_cast<VarDecl>(expr->getDecl())) != IDDepVars.end()) {
      return expr;
    }
    return NULL;
  }
  // If we care about thread-dependent array subscript exprs, turn off the below
  if (auto *ase = dyn_cast<ArraySubscriptExpr>(e))
    return NULL;
  // We need to iterate over its children and see if any of them are
  for (auto i = e->child_begin(); i != e->child_end(); ++i) {
    if (auto *newExpr = dyn_cast<Expr>(*i)) {
      auto *retExpr = hasIDDepDeclRef(newExpr);
      if (retExpr)
        return retExpr;
    }
  }
  // If none of the children force an early return with a match, return NULL
  return NULL;
}

const MemberExpr *
PossiblyUnreachableBarrierCheck::hasIDDepMember(const Expr *e) {
  if (const MemberExpr *expr = dyn_cast<MemberExpr>(e)) {
    // It is a DeclRefExpr, so check if it's an ID-dependent variable
    if (std::find(IDDepFields.begin(), IDDepFields.end(),
                  dyn_cast<FieldDecl>(expr->getMemberDecl())) !=
        IDDepFields.end()) {
      return expr;
    }
    return NULL;
  }
  // If we care about thread-dependent array subscript exprs, turn off the below
  // if (auto * ase = dyn_cast<ArraySubscriptExpr>(e)) return NULL;
  // We need to iterate over its children and see if any of them are
  for (auto i = e->child_begin(); i != e->child_end(); ++i) {
    if (auto *newExpr = dyn_cast<Expr>(*i)) {
      auto *retExpr = hasIDDepMember(newExpr);
      if (retExpr)
        return retExpr;
    }
  }
  // If none of the children force an early return with a match, return NULL
  return NULL;
}

bool PossiblyUnreachableBarrierCheck::isElseIfStmt(const MatchFinder::MatchResult &Result, const IfStmt *IfStatement) {
  clang::ASTContext::DynTypedNodeList Parents = Result.Context->getParents(*IfStatement);
  if (Parents.empty()) {
    return false;
  }
  const Stmt* Parent = Parents[0].get<Stmt>();
  if (Parent) {
    const IfStmt* IfParent = dyn_cast<IfStmt>(Parent);
    if (IfParent && IfParent->getElse() == IfStatement) {
      return true;
    }
  }
  return false;
}

bool flattenIfElse(const IfStmt *IfStatement, llvm::SmallVector<const Stmt *, 10> &IfElseStmts) {
  const IfStmt * CurrentIfStmt = IfStatement;
  // IfAnsc->dump();
  IfElseStmts.push_back(CurrentIfStmt->getThen());
  const Stmt *NextIfStmt = IfStatement->getElse();
  for (; NextIfStmt != NULL && dyn_cast<IfStmt>(NextIfStmt);
       NextIfStmt = CurrentIfStmt->getElse()) {
    // If we are here then nextIf contains an IfStmt for an else-if branch
    CurrentIfStmt = dyn_cast<IfStmt>(NextIfStmt);
    IfElseStmts.push_back(CurrentIfStmt->getThen());
  }
  if (NextIfStmt == NULL) { // There is no final else
    // then we cannot prove all threads execute the barrier, move on to
    // diagnostics
    return true;
  }
  // nextIf holds the actual final else statement
  IfElseStmts.push_back(NextIfStmt);
  return false;
}

bool PossiblyUnreachableBarrierCheck::isFalsePositiveIfStmt(const MatchFinder::MatchResult &Result, const IfStmt *IfAnsc) {
  // If it is an else-if statement, do not evaluate
  if (isElseIfStmt(Result, IfAnsc))
    return true;

  llvm::SmallVector<const Stmt *, 10> checks;
  if (flattenIfElse(IfAnsc, checks))
    return false;

  // The list of branches to check is complete make sure they all call a
  // simple barrier the same number of times As soon as we find a non-simple
  // or uneven barrier, break to diags Only if we can guarantee all barriers
  // are simple and evenly hit can we early-return to remove the
  // false-positive
  std::list<const Stmt *> FlatIfElse;
  preorderFlattenStmt(IfAnsc, &FlatIfElse);
  // FlatIfElse now contains all children of If statement.
  size_t CurrentIfElseStmt = 0;
  int TargetNumBarriers = 0;
  int CurrentNumBarriers = 0;
  for (const Stmt * Statement : FlatIfElse) {
    // If Statement is the next If/Else Stmt, or the last Stmt in FlatIfElse.
    if (Statement == FlatIfElse.back() || 
        Statement == checks[CurrentIfElseStmt + 1]) {
      // If looking at the first If statement, record the number of barriers
      // it has.
      if (CurrentIfElseStmt == 0) {
        TargetNumBarriers = CurrentNumBarriers;
      }
      // If the number of barriers doesn't match, then this is definitely not
      // a false positive.
      if (TargetNumBarriers != CurrentNumBarriers) {
        return false;  // Number of barriers isn't the same in every check.
      }
      CurrentNumBarriers = 0;  // Reset barrier count.
      CurrentIfElseStmt++;  // Move to next If/Else statement.
    }
    // If Statement is a branch, cannot prove it is a false positive.
    if (isa<IfStmt>(Statement) || isa<SwitchStmt>(Statement) ||
        isa<ForStmt>(Statement) || isa<DoStmt>(Statement) ||
        isa<WhileStmt>(Statement) || isa<ReturnStmt>(Statement) ||
        isa<BreakStmt>(Statement) || isa<GotoStmt>(Statement)) {
      // FIXME: recursively check if all cases of this branch have the same
      // number of barrier calls instead of simply returning 'false'  
      return false;
    }
  }
  // All branches have the same number of barriers, so it is a false positive
  return true;
}

void PossiblyUnreachableBarrierCheck::check(
    const MatchFinder::MatchResult &Result) {
  SourceManager &ResultSM = Result.Context->getSourceManager();
  findIDDependentVariablesAndFields(Result);

  // We want the diagnostic to emit slightly different text so we bind on the
  // five potential conditionals (of which only one will be true) a barrier and
  // conditional expreesion, and possibly an id call
  const auto *BarrierCall = Result.Nodes.getNodeAs<CallExpr>("barrier");
  const auto *ForAnsc = Result.Nodes.getNodeAs<ForStmt>("for");
  const auto *IfAnsc = Result.Nodes.getNodeAs<IfStmt>("if");
  const auto *DoAnsc = Result.Nodes.getNodeAs<DoStmt>("do");
  const auto *WhileAnsc = Result.Nodes.getNodeAs<WhileStmt>("while");
  const auto *SwitchAnsc = Result.Nodes.getNodeAs<SwitchStmt>("switch");
  const auto *CondExpr = Result.Nodes.getNodeAs<Expr>("cond_expr");
  const auto *IDCall = Result.Nodes.getNodeAs<CallExpr>("id_call");

  // Figure out which type of conditional we have for the select in the later
  // diagnostics
  BranchType Branch = UNKNOWN;
  if (ForAnsc) {
    Branch = FOR_LOOP;
  } else if (IfAnsc) {
    Branch = IF_STMT;
  } else if (DoAnsc) {
    Branch = DO_LOOP;
  } else if (WhileAnsc) {
    Branch = WHILE_LOOP;
  } else if (SwitchAnsc) {
    Branch = SWITCH_STMT;
  }

  if (CondExpr) {
    // Before we emit a diagnostic, see if we can quickly conclusively prove
    // it's a false positive
    switch (Branch) {
    case FOR_LOOP:   // handle for loop
    case DO_LOOP:    // handle do loop
    case WHILE_LOOP: // handle while loop
      // TODO: If it's a loop, make sure it's not unconditionally executed a
      // non-ID-dependent number of times (constant offset from TID)
      break;

    // TODO: Needs to handle breaks, returns, and gotos
    case IF_STMT: { // handle if statement
      if (isFalsePositiveIfStmt(Result, IfAnsc))
        return;
    } break;

    // TODO: needs to handle returns and gotos
    case SWITCH_STMT: { // handle switch statement
      // TODO: If it's a switch, recursively check that all cases (and a
      // default) have a barrier
      bool hasDefault = false;
      // llvm::errs() << "Diagnosing SwitchStmt";
      // SwitchAnsc->dump();
      const SwitchCase *cases = SwitchAnsc->getSwitchCaseList();
      std::list<const Stmt *> checks;
      // This iterates from the bottom of the case list, upwards
      for (; cases != NULL; cases = cases->getNextSwitchCase()) {
        if (dyn_cast<DefaultStmt>(cases)) {
          // llvm::errs() << "found default case\n";
          hasDefault = true;
        }
        // Immediately-nested case statements just represent consecutive cases
        // without a break or intervening statement, so we don't need to
        // explicitly check them (as they will show up independently)
        if (!dyn_cast<CaseStmt>(cases->getSubStmt()))
          checks.push_back(cases);
        // We do however need to check for consecutive cases without an
        // intervening break; cases->getSubStmt()->dump(); cases->dump();
      }
      // If none of the cases are a default we can't guarantee that all threads
      // have a chance to hit a barrier, break to diagnostics llvm::errs() <<
      // "hasDefault: " << hasDefault << "\n";
      if (hasDefault == false)
        break;
      // At this point checks should have all the case statements that include
      // logic So we need to iterate through each start Stmt until the next
      // BreakStmt is found (w.r.t the SwitchAnsc (inorder traversal of all
      // descendants of SwitchAnsc)
      // TODO: Do we want to iterate over checks itself or SwitchAnsc?
      // Technically we will have tu jump up the tree to catch some statements
      // if there is more than one (not in a CompoundStmt) for a case (counting
      // BreakStmt)
      std::list<const Stmt *> flatSwitch;
      preorderFlattenStmt(SwitchAnsc->getBody(), &flatSwitch);
      checks.reverse();
      std::list<const Stmt *>::iterator currCase = checks.begin();
      int maxBarriers = -1;
      int currBarriers = 0;
      for (auto itr = flatSwitch.begin();
           itr != flatSwitch.end() && currCase != checks.end(); itr++) {
        // printf("Evaluating Stmt: %x  currCase: %x\n", *itr, *currCase);
        // Iterate until we hit currCase
        if (*itr == *currCase) {
          // whatever we need to do to start a new check
          // llvm::errs() << "Found currCase, advancing\n";
          // currCase++
          currBarriers = 0;
        }
        // If we're at a subsequent case but haven't been advanced from a
        // breakstatment
        //		printf("Checking reverse hack %x %x\n", *currCase,
        //*(currCase.base()-1L)); We really want if (*itr == *(currCase+1)) but
        // it doesn't compile correctly if (*itr == *((currCase.base())[-1])) {
        if (*itr == *(std::next(currCase, 1))) {
          // llvm::errs() << "Found subsequent case without break, advancing\n";
          if (currBarriers > 0) {
            // llvm::errs() << "But carryover case has barrier, aborting\n";
            break;
          }
          currCase++;
        }
        // If we reach a BreakStmt without hitting at least 1 barrier, break
        // If this isn't the first evaluated case, and we hit a different number
        // of barriers than the previous case(s), break;
        if (dyn_cast<BreakStmt>(*itr)) {
          // End of a case
          if (currBarriers == 0) {
            // llvm::errs() << "Aborting due to barrier-less case\n";
            break; // Break out of the loop (if we are even looking at this
                   // switch at least one case has a barrier
          }
          if (maxBarriers != -1 && currBarriers != maxBarriers) {
            // llvm::errs() << "Aborting due to case with non-matching barrier
            // count! this: " << currBarriers << " previous: " << maxBarriers <<
            // "\n";
            break; // Some other case has a different number of consecutive
                   // barriers
          }
          if (maxBarriers == -1)
            maxBarriers =
                currBarriers; // This is the first case evaluated, set the
                              // number of expected barriers for everyone else
          // If this case has passed, move on to the next
          currCase++;
          // If we just processed our last case and haven't failed yet, then we
          // can assume the barrier is safe and return!
          if (currCase == checks.end()) {
            // llvm::errs() << "Checked all cases and barriers are provably
            // executed by all routes\n";
            return;
          }
        }
        // Start recording barrier calls
        if (auto call = dyn_cast<CallExpr>(*itr)) {
          // TODO: CFG check to see if the function is known to call a barrier
          if (const FunctionDecl *callDecl = call->getDirectCallee()) {
            std::string name = callDecl->getNameAsString();
            // If we call any extra functions, break
            if (name != "" && name != "barrier" &&
                name != "work_group_barrier") {
              // llvm::errs() << "Aborting due to non-barrier call, cannot check
              // CFG for barrier\n";
              break; // If we call some non-barrier function, it may have a
                     // barrier, abort
              // TODO: Loosen this restriction so that non-barrier OpenCL
              // builtins can be called.
            } else {
              currBarriers++;
            }
          } else {
            // What are we calling? diag and abort
            diag(call->getBeginLoc(),
                 "Cannot deduce type of call in switch case, flagging switch "
                 "for potentially unreachable barrier");
            // call->dump();
            break;
          }
        }
        // If we hit any branches, break
        if (isa<IfStmt>(*itr) || isa<SwitchStmt>(*itr) || isa<ForStmt>(*itr) ||
            isa<DoStmt>(*itr) || isa<WhileStmt>(*itr) ||
            isa<ReturnStmt>(*itr) || isa<GotoStmt>(*itr)) {
          diag((*itr)->getBeginLoc(),
               "Cannot guarantee barrier due to nested branch or jump");
          break;
        }
        // If we hit any gotos, returns or other jumps, break
      }
    } break;

    default:
      diag(CondExpr->getBeginLoc(), "Conditional in undiagnosable branch type, "
                                    "cannot check for barriers");
      break;
    }
    // If there's a get_global/local_id call
    if (IDCall) {
      // It calls one of the ID functions directly
      diag(
          BarrierCall->getBeginLoc(),
          "Barrier inside %select{for loop|if/else|do loop|while loop|switch}0 "
          "may not be reachable due to ID function call in condition at %1")
          << Branch << CondExpr->getBeginLoc().printToString(ResultSM);
    } else {
      // It has some DeclRefExpr(s), check for ID-dependency
      const auto *retDeclExpr = hasIDDepDeclRef(CondExpr);
      const auto *retMemberExpr = hasIDDepMember(CondExpr);
      if (retDeclExpr) {
        // It has an ID-dependent reference
        diag(BarrierCall->getBeginLoc(),
             "Barrier inside %select{for loop|if/else|do loop|while "
             "loop|switch}0 may not be reachable due to reference to "
             "ID-dependent variable %2 in condition at %1")
            << Branch << CondExpr->getBeginLoc().printToString(ResultSM)
            << retDeclExpr->getDecl();
      } else if (retMemberExpr) {
        // It has an ID-dependent reference
        diag(BarrierCall->getBeginLoc(),
             "Barrier inside %select{for loop|if/else|do loop|while "
             "loop|switch}0 may not be reachable due to reference to "
             "ID-dependent member %2 in condition at %1")
            << Branch << CondExpr->getBeginLoc().printToString(ResultSM)
            << retMemberExpr->getMemberDecl();
      } else {
        // Do nothing, there's nothing wrong with a non-ID-dependent conditional
        // expression
      }
    }
  }
}

void PossiblyUnreachableBarrierCheck::preorderFlattenStmt(
    const Stmt *s, std::list<const Stmt *> *out) {
  // TODO: Flatten a Stmt and all descendants recursively
  // Record ourselves, then iterate over all children
  // printf("flattening %x %x %x\n", s, s->child_begin(), s->child_end());
  out->push_back(s);
  for (auto itr = s->child_begin(); itr != s->child_end(); itr++) {
    if (*itr != NULL)
      preorderFlattenStmt(*itr, out);
  }
}

} // namespace OpenCL
} // namespace tidy
} // namespace clang
