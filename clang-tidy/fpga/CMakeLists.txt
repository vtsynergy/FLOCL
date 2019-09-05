set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyFPGAModule
  FPGATidyModule.cpp
  StructPackAlignCheck.cpp
  
  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyUtils
  )
