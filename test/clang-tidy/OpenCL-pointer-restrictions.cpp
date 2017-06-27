// RUN: %check_clang_tidy %s OpenCL-pointer-restrictions %t -- -header-filter=.* "--" --include opencl-c.h -cl-std=CL1.2 -c 

//TODO Customize checks so that 1.2 and 2.0 can be checked in tandem (i.e. support the generic address space)

int func(int x) {
	return x*x;
} 

// FIXME: Add something that triggers the check here.
__kernel void errors(int * err1, __global int * succ1, __constant int * succ2, __local int * succ3, __global int ** err5) {
// CHECK-MESSAGES: :[[@LINE-1]]:28: warning: OpenCL kernel param 'err1' with pointer type must reside in either the __global, __constant, or __local address space. [OpenCL-pointer-restrictions]
// CHECK-MESSAGES: :[[@LINE-2]]:28: error: kernel parameter cannot be declared as a pointer to the __private address space [clang-diagnostic-error]
// CHECK-MESSAGES: :[[@LINE-3]]:117: warning: Kernel parameters may not be declared as pointers to pointers before OpenCL 2.0.
// CHECK-MESSAGES: :[[@LINE-4]]:117: warning: OpenCL kernel param 'err5' with pointer type must reside in either the __global, __constant, or __local address space. [OpenCL-pointer-restrictions]
// CHECK-MESSAGES: :[[@LINE-5]]:117: error: kernel parameter cannot be declared as a pointer to a pointer [clang-diagnostic-error]
  __global int * err2 = succ2;
// CHECK-MESSAGES: :[[@LINE-1]]:18: error: initializing '__global int *' with an expression of type '__constant int *' changes address space of pointer [clang-diagnostic-error]
  err2 = succ3;
// CHECK-MESSAGES: :[[@LINE-1]]:8: error: assigning '__local int *' to '__global int *' changes address space of pointer [clang-diagnostic-error]
  __constant int * err3 = succ3;
// CHECK-MESSAGES: :[[@LINE-1]]:20: error: initializing '__constant int *' with an expression of type '__local int *' changes address space of pointer [clang-diagnostic-error]
  err3 = succ1;
// CHECK-MESSAGES: :[[@LINE-1]]:8: error: assigning '__global int *' to '__constant int *' changes address space of pointer [clang-diagnostic-error]
  __local int * err4 = succ1;
// CHECK-MESSAGES: :[[@LINE-1]]:17: error: initializing '__local int *' with an expression of type '__global int *' changes address space of pointer [clang-diagnostic-error]
  err4 = succ2;
// CHECK-MESSAGES: :[[@LINE-1]]:8: error: assigning '__constant int *' to '__local int *' changes address space of pointer [clang-diagnostic-error]
  int (*err6)(int) = &func;
// CHECK-MESSAGES: :[[@LINE-1]]:9: error: pointers to functions are not allowed [clang-diagnostic-error]
// CHECK-MESSAGES: :[[@LINE-2]]:23: error: taking address of function is not allowed [clang-diagnostic-error]
}


__kernel void success(__global int * succ1, __constant int * succ2, __local int * succ3) {
  __global int * succ4 = succ1;
  __constant int * succ5 = succ2;
  __local int * succ6 = succ3;
}
