set(LLVM_LINK_COMPONENTS
  support
  )

get_filename_component(CLANG_LINT_SOURCE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/../../clang-tidy REALPATH)
include_directories(${CLANG_LINT_SOURCE_DIR})

add_extra_unittest(ClangTidyTests
  ClangTidyDiagnosticConsumerTest.cpp
  ClangTidyOptionsTest.cpp
  IncludeInserterTest.cpp
  GoogleModuleTest.cpp
  LLVMModuleTest.cpp
  NamespaceAliaserTest.cpp
  ObjCModuleTest.cpp
  OverlappingReplacementsTest.cpp
  UsingInserterTest.cpp
  ReadabilityModuleTest.cpp)

if(${LLVM_PACKAGE_VERSION} VERSION_LESS "9.0.0")
	add_extra_unittest(ClangTidyTests ExprMutationAnalyzerTest.cpp)
endif(${LLVM_PACKAGE_VERSION} VERSION_LESS "9.0.0")

target_link_libraries(ClangTidyTests
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangLex
  clangSerialization
  clangTidy
  clangTidyAndroidModule
  clangTidyGoogleModule
  clangTidyLLVMModule
  clangTidyObjCModule
  clangTidyReadabilityModule
  clangTidyUtils
  clangTooling
  clangToolingCore
  )
