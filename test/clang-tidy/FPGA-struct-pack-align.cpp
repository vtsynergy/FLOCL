// RUN: %check_clang_tidy %s FPGA-struct-pack-align %t -- -header-filter=.* "--" --include opencl-c.h -cl-std=CL1.2 -c 

// FIXME: Add something that triggers the check here.
struct error {
char a;
double b;
char c;
};
// CHECK-MESSAGES: :[[@LINE-5]]:8: warning: struct 'error' has inefficient access due to padding, only needs 10 bytes but is using 24 bytes, use "__attribute((packed))" [FPGA-struct-pack-align]
// CHECK-MESSAGES: :[[@LINE-6]]:8: warning: struct 'error' has inefficient access due to poor alignment. Currently aligned to 8 bytes, but size 10 bytes is large enough to benefit from "__attribute((aligned(16)))" [FPGA-struct-pack-align]



struct success{
char a;
double b;
char c;
} __attribute((packed)) __attribute((aligned(16)));
//Should take 10 bytes and be aligned to 16 bytes


