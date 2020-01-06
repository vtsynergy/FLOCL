set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyAlteraModule
  AlteraTidyModule.cpp
  SingleWorkItemBarrierCheck.cpp

  LINK_LIBS
  clangAnalysis
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyUtils
  )
