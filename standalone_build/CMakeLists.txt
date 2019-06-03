#Testing out-of-source build, only tested on Ubuntu 18.04 with Apt libclang-6.0-dev install

#required variable configurations
#set(LIBRARY_DIR "/usr/lib/llvm-6.0/lib")
set(LLVM_MAIN_INCLUDE_DIR "/usr/lib/llvm-6.0/include")
include_directories(${LLVM_MAIN_INCLUDE_DIR})
#set(INCLUDE_DIR "/usr/lib/llvm-6.0/include")
#set(LLVM_BINARY_DIR "/usr/lib/llvm-6.0/")
#set(LLVM_CMAKE_PATH "usr/lib/llvm-6.0/lib/cmake/llvm")
#set(LLVM_ALL_TARGETS "clang-tidy")
#set(LLVM_TARGETS_TO_BUILD "clang-tidy")
if(POLICY CMP0057)
	cmake_policy(SET CMP0057 NEW)
endif()
if(POLICY CMP0022)
	cmake_policy(SET CMP0022 NEW)
endif()

#dependencies
list(APPEND CMAKE_MODULE_PATH "/usr/lib/llvm-6.0/lib/cmake/llvm/")
include(LLVMConfig)
include(LLVMExports)
include(AddLLVM)
#from https://raw.github.com/llvm-mirror/clang/master/cmake/modules/AddClang.cmake
include(AddClang.cmake)
string(REPLACE "." "" INT_LLVM_PACKAGE_VERSION ${LLVM_PACKAGE_VERSION})
add_definitions(-DLLVM_PACKAGE_VERSION=${INT_LLVM_PACKAGE_VERSION})

#include(../CMakeLists.txt)
#add_subdirectory(flocl)
add_subdirectory(flocl/clang-tidy)
