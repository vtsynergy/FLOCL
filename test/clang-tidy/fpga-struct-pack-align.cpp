// RUN: %check_clang_tidy %s fpga-struct-pack-align %t -- -header-filter=.* "--" --include opencl-c.h -cl-std=CL1.2 -c 

// Struct needs both alignment and packing
struct error {
char a;
double b;
char c;
};
// CHECK-MESSAGES: :[[@LINE-5]]:8: warning: struct 'error' has inefficient access due to padding, only needs 10 bytes but is using 24 bytes, use "__attribute((packed))" [fpga-struct-pack-align]
// CHECK-MESSAGES: :[[@LINE-6]]:8: warning: struct 'error' has inefficient access due to poor alignment. Currently aligned to 8 bytes, but size 10 bytes is large enough to benefit from "__attribute((aligned(16)))" [fpga-struct-pack-align]

// Struct is explicitly packed, but needs alignment
struct error_packed {
char a;
double b;
char c;
} __attribute((packed));
// CHECK-MESSAGES: :[[@LINE-5]]:8: warning: struct 'error_packed' has inefficient access due to poor alignment. Currently aligned to 1 bytes, but size 10 bytes is large enough to benefit from "__attribute((aligned(16)))" [fpga-struct-pack-align]

// Struct is properly packed, but needs alignment
struct align_only {
char a;
char b;
char c;
char d;
int e;
double f;
};
// CHECK-MESSAGES: :[[@LINE-8]]:8: warning: struct 'align_only' has inefficient access due to poor alignment. Currently aligned to 8 bytes, but size 16 bytes is large enough to benefit from "__attribute((aligned(16)))" [fpga-struct-pack-align]

// Struct is perfectly packed but wrongly aligned
struct bad_align {
char a;
double b;
char c;
} __attribute((packed)) __attribute((aligned(8)));
// CHECK-MESSAGES: :[[@LINE-5]]:8: warning: struct 'bad_align' has inefficient access due to poor alignment. Currently aligned to 8 bytes, but size 10 bytes is large enough to benefit from "__attribute((aligned(16)))" [fpga-struct-pack-align]

struct bad_align2 {
char a;
double b;
char c;
} __attribute((packed)) __attribute((aligned(32)));
// CHECK-MESSAGES: :[[@LINE-5]]:8: warning: struct 'bad_align2' has inefficient access due to poor alignment. Currently aligned to 32 bytes, but size 10 bytes is large enough to benefit from "__attribute((aligned(16)))" [fpga-struct-pack-align]

struct bad_align3 {
char a;
double b;
char c;
} __attribute((packed)) __attribute((aligned(4)));
// CHECK-MESSAGES: :[[@LINE-5]]:8: warning: struct 'bad_align3' has inefficient access due to poor alignment. Currently aligned to 4 bytes, but size 10 bytes is large enough to benefit from "__attribute((aligned(16)))" [fpga-struct-pack-align]

// Struct is both perfectly packed and aligned
struct success {
char a;
double b;
char c;
} __attribute((packed)) __attribute((aligned(16)));
//Should take 10 bytes and be aligned to 16 bytes

// Struct is properly packed, and explicitly aligned
struct success2 {
int a;
int b;
int c;
} __attribute((aligned(16)));

// If struct is properly aligned, packing not needed
struct error_aligned {
char a;
double b;
char c;
} __attribute((aligned(16)));

