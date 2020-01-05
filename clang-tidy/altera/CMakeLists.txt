set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyAlteraModule
  AlteraTidyModule.cpp
  KernelNameRestrictionCheck.cpp

  LINK_LIBS
  clangAnalysis
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyUtils
  )
