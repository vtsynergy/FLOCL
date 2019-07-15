set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyFPGAModule
  FPGATidyModule.cpp
  IdDependentBackwardBranchCheck.cpp
  KernelNameRestrictionCheck.cpp
  SingleWorkItemBarrierCheck.cpp
  StructPackAlignCheck.cpp
  UnrollLoopsCheck.cpp
  
  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyUtils
  )
