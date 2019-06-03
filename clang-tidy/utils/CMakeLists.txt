set(LLVM_LINK_COMPONENTS support)

list(APPEND CLANG_TIDY_UTILS_SRC 
  ASTUtils.cpp
  DeclRefExprUtils.cpp
  ExceptionAnalyzer.cpp
  ExprSequence.cpp
  FixItHintUtils.cpp
  HeaderFileExtensionsUtils.cpp
  HeaderGuard.cpp
  IncludeInserter.cpp
  IncludeSorter.cpp
  LexerUtils.cpp
  NamespaceAliaser.cpp
  OptionsUtils.cpp
  TypeTraits.cpp
  UsingInserter.cpp
)
if(${LLVM_PACKAGE_VERSION} VERSION_LESS "9.0.0")
	list(APPEND CLANG_TIDY_UTILS_SRC ExprMutationAnalyzer.cpp)
endif(${LLVM_PACKAGE_VERSION} VERSION_LESS "9.0.0")
add_clang_library(clangTidyUtils

  ${CLANG_TIDY_UTILS_SRC}

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  )
