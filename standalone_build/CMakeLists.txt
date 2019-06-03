#Testing out-of-source build, only tested on Ubuntu 18.04 with Apt libclang-6.0-dev install
#This is really hacky and is likely non-portable

#required variable configurations
set(LLVM_MAIN_INCLUDE_DIR "/usr/lib/llvm-6.0/include")
include_directories(${LLVM_MAIN_INCLUDE_DIR})
set(LLVM_LIBRARY_DIR "/usr/lib/llvm-6.0/lib")
set(CLANG_LIBRARY_DIR "/usr/lib/llvm-6.0/lib")
link_directories(${LLVM_LIBRARY_DIR})
set(CMAKE_CXX_FLAGS "-fno-rtti")
if(POLICY CMP0057)
	cmake_policy(SET CMP0057 NEW)
endif()
if(POLICY CMP0022)
	cmake_policy(SET CMP0022 NEW)
endif()
if(POLICY CMP0003)
	cmake_policy(SET CMP0003 NEW)
endif()

#dependencies
list(APPEND CMAKE_MODULE_PATH "/usr/lib/llvm-6.0/lib/cmake/llvm/")
include(LLVMConfig)
#include(LLVMExports)
include(AddLLVM)
#from https://raw.github.com/llvm-mirror/clang/master/cmake/modules/AddClang.cmake
include(AddClang.cmake)
string(REPLACE "." "" INT_LLVM_PACKAGE_VERSION ${LLVM_PACKAGE_VERSION})
add_definitions(-DLLVM_PACKAGE_VERSION=${INT_LLVM_PACKAGE_VERSION})
#from /usr/share/llvm-6.0/cmake with modifications (it's unusable directly from disk but specifies the dependency information needed to link all the sub libraries in the correct order)
include(ClangTargets.cmake)
set(CLANG_ENABLE_STATIC_ANALYZER TRUE)
add_subdirectory(flocl/clang-tidy)
