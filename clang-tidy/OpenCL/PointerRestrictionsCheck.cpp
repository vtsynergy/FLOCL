//===--- PointerRestrictionsCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PointerRestrictionsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace OpenCL {

//Implement a matcher to check whether a variable is in a specific address space
AST_MATCHER_P(QualType, hasAddrSpace, unsigned int, addrSpace) {
     return ((unsigned)Node.getAddressSpace() == addrSpace);
}

void PointerRestrictionsCheck::registerMatchers(MatchFinder *Finder) {
  //Arguments to __kernel functions declared in a program that are pointers must be declared with the __global, __constant or __local qualifier.
  Finder->addMatcher(
    parmVarDecl(allOf(
      hasAncestor(
        functionDecl(
          hasAttr(attr::Kind::OpenCLKernel)
        )
      ),
      hasType(isAnyPointer()),
      unless(anyOf(
        hasType(pointsTo(hasAddrSpace((unsigned)LangAS::opencl_global))),
        hasType(pointsTo(hasAddrSpace((unsigned)LangAS::opencl_constant))),
        hasType(pointsTo(hasAddrSpace((unsigned)LangAS::opencl_local)))
      ))
    )).bind("ptr_param_addrspace"), this);
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
  const auto ADDRSPACE_RHS = expr(hasDescendant(declRefExpr(allOf(
        hasType(isAnyPointer()),
        anyOf(
          hasType(pointsTo(hasAddrSpace((unsigned)LangAS::opencl_global))),
          hasType(pointsTo(hasAddrSpace((unsigned)LangAS::opencl_constant))),
          hasType(pointsTo(hasAddrSpace((unsigned)LangAS::opencl_local)))
        ),
        unless(hasType(pointsTo(isAnyPointer())))
      )).bind("ptr_assign_RHS")));
  //A pointer declared with the __constant, __local, or __global qualifier can only be assigned to a pointer declared with the __constant, __local, or __global qualifier respectively. (v1.0, 1.1, 1.2)
  //Need to get DeclRefExprs to pointer types that have one of the affected attrs, and check whether they are being assigned to something, if so, check it's qualified type for a matching ATTR
  //FIXME, This is not catching assignments from mixed address spaces like it's supposed to
  // but clang already emits an error so is it important?
  Finder->addMatcher(
    stmt(anyOf(
      declStmt(hasDescendant(
        varDecl(
          hasInitializer(ADDRSPACE_RHS),
          hasType(isAnyPointer())
        ).bind("ptr_assign_LHS")
      )),
      binaryOperator(allOf(
        ANY_ASSIGN,
        hasRHS(ADDRSPACE_RHS),
        hasLHS(expr(hasDescendant(declRefExpr(
          hasType(isAnyPointer()),
          to(decl().bind("ptr_assign_LHS"))
        ))))
      ))
    )).bind("ptr_assign_stmt"), this);
  //Pointers to functions are not allowed. (all versions)
  // Just make a matcher that has type pointer and some other stuff (what does a function pointer look like on the AST?
  //FIXME This i not catching function pointers in kernels like it's supposed to, but clang already emits an error so is it important?
  Finder->addMatcher(
    valueDecl(allOf(
      hasType(isAnyPointer()),
      anyOf(
        hasType(pointsTo(functionDecl())),
        hasType(functionType())
      )
    )).bind("ptr_functionPtr_var"), this);
  //Arguments to __kernel functions in a program cannot be declared as a pointer to a pointer(s). Variables inside a function or arguments to non __kernel functions in a program can be declared as a pointer to a pointer(s). (v1.0, 1.1, 1.2)
  // Bind on parmVarDecls that have type pointer to pointer
  Finder->addMatcher(
    parmVarDecl(allOf(
      hasType(isAnyPointer()),
      hasType(pointsTo(isAnyPointer()))
    )).bind("ptr_ptrToPtr_parm"), this);
}

void PointerRestrictionsCheck::check(const MatchFinder::MatchResult &Result) {
  //Callback to make sure all pointer params have an address space
  const auto *ParmAddrSpaceDecl = Result.Nodes.getNodeAs<ParmVarDecl>("ptr_param_addrspace");
  if (ParmAddrSpaceDecl) {
    diag(ParmAddrSpaceDecl->getLocation(), "OpenCL kernel param %0 with pointer type must reside in either the __global, __constant, or __local address space.")
      << ParmAddrSpaceDecl;
  }
  //Callback to make sure any pointer assignments have appropriate address space
  //TODO: check if OpenCL version is >= 2.0 and allow generic address space for global/local
  const auto * PtrAssignOper = Result.Nodes.getNodeAs<Stmt>("ptr_assign_stmt");
  const auto * PtrAssignRHS = Result.Nodes.getNodeAs<Expr>("ptr_assign_RHS");
  const auto * PtrAssignLHS = Result.Nodes.getNodeAs<VarDecl>("ptr_assign_LHS");
  if (PtrAssignOper && PtrAssignRHS && PtrAssignLHS) {
    switch (PtrAssignRHS->getType().getAddressSpace()) {
      case LangAS::opencl_global:
        if (Result.Context->getLangOpts().OpenCLVersion >= 200 && PtrAssignLHS->getType().getAddressSpace() == LangAS::opencl_generic ) {
          break;
        }
        if (PtrAssignLHS->getType().getAddressSpace() == LangAS::opencl_global ) {
          break;
        }
        diag(PtrAssignOper->getBeginLoc(), "Pointers in OpenCL global address space can only be assigned to pointer variables in the OpenCL global address space (or generic address space with OpenCL >= 2.0).");
        break;
      case LangAS::opencl_local:
        if (Result.Context->getLangOpts().OpenCLVersion >= 200 && PtrAssignLHS->getType().getAddressSpace() == LangAS::opencl_generic ) {
          break;
        }
        if (PtrAssignLHS->getType().getAddressSpace() == LangAS::opencl_local ) {
          break;
        }
        diag(PtrAssignOper->getBeginLoc(), "Pointers in OpenCL local address space can only be assigned to pointer variables in the OpenCL local address space (or generic address space with OpenCL >= 2.0).");
        break;
      case LangAS::opencl_constant:
        if (PtrAssignLHS->getType().getAddressSpace() != LangAS::opencl_constant) {
          diag(PtrAssignOper->getBeginLoc(), "Pointers in OpenCL constant address space can only be assigned to pointer variables also in the OpenCL constant address space.");
        }
        break;
    }
  }
  //Callbak to make sure no pointers to functions are being used
  const auto * PtrFuncPtr = Result.Nodes.getNodeAs<ValueDecl>("ptr_functionPtr_var");
  if (PtrFuncPtr) {
    diag(PtrFuncPtr->getLocation(), "Function pointers are not allowed in OpenCL kernels");
  }
  //Callback to ensure kernel parameters are not declared as pointers to pointers in OpenCL < 2.0
  const auto * PtrToPtr = Result.Nodes.getNodeAs<ParmVarDecl>("ptr_ptrToPtr_parm");
  if (PtrToPtr && Result.Context->getLangOpts().OpenCLVersion < 200) {
    diag(PtrToPtr->getLocation(), "Kernel parameters may not be declared as pointers to pointers before OpenCL 2.0.");
  }
}

} // namespace OpenCL
} // namespace tidy
} // namespace clang
