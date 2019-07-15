// RUN: %check_clang_tidy -expect-clang-tidy-error %s opencl-recursion-not-supported %t -- -config="{CheckOptions: [{key: "opencl-recursion-not-supported.MaxRecursionDepth", value: 3}]}" -header-filter=.* "--" --include opencl-c.h -cl-std=CL1.2 -c

// Simple recursive function should trigger an error
void recfun() {
  recfun();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: error: The call to function recfun is recursive, which is not supported by OpenCL.
  // CHECK-NEXT: recfun is called by recfun in {{.*:\d+:\d+}}
}

// Declare functions first
void recfun1();
void recfun2();
void recfun3();

void recfundeep1();
void recfundeep2();
void recfundeep3();
void recfundeep4();

// Recursive function with depth 3 should trigger error
void recfun1() {
  recfun2();
}

void recfun2() {
  recfun3();
}

void recfun3() {
  recfun1();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: error: The call to function recfun1 is recursive, which is not supported by OpenCL.
  // CHECK-NEXT: recfun1 is called by recfun3 in {{.*:\d+:\d+}}
  // CHECK-NEXT: recfun3 is called by recfun2 in {{.*:\d+:\d+}}
  // CHECK-NEXT: recfun2 is called by recfun1 in {{.*:\d+:\d+}}
}

// Non-recursive function should not trigger an error
int nonrecursivefun() {
  return 100; 
}

// Recursive function with depth greater than 3 should not trigger an error
void recfundeep1() {
  recfundeep2();
}

void recfundeep2() {
  recfundeep3();
}

void recfundeep3() {
  recfundeep4();
}

void recfundeep4() {
  recfundeep1();
}
