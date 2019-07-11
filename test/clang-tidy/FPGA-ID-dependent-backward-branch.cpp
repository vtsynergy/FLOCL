// RUN: %check_clang_tidy %s FPGA-ID-dependent-backward-branch %t -- -header-filter=.* "--" -cl-std=CL1.2 -c --include opencl-c.h

typedef struct ExampleStruct {
  int IDDepField;
} ExampleStruct;

void error() {
  // ==== Assignments ====
  int tid = get_local_id(0);
// CHECK-NOTES: :[[@LINE-1]]:3: note: assignment of ID-dependent variable 'tid' declared at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:3 [FPGA-ID-dependent-backward-branch]

  ExampleStruct Example;
  Example.IDDepField = get_local_id(0);
  // Will not produce warning, but probably should

  // ==== Inferred Assignments ====
  int tx = tid * get_local_size(0);
// CHECK-NOTES: :[[@LINE-1]]:12: note: Inferred assignment of ID-dependent value from ID-dependent variable 'tid' [FPGA-ID-dependent-backward-branch]
  int tx2 = Example.IDDepField;
// CHECK-NOTES: :[[@LINE-1]]:13: note: Inferred assignment of ID-dependent value from ID-dependent member 'IDDepField' [FPGA-ID-dependent-backward-branch]

  ExampleStruct Example2 = {
    tid * 2
  };

  // ExampleClass Example3(tid);
  // Example2.IDDepField = Example.IDDepField * 2;
  // ==== Conditional Expressions ====
  int accumulator = 0;
  for (int i = 0; i < get_local_id(0); i++) {
    // CHECK-NOTES: :[[@LINE-1]]:23: warning: Backward branch (for loop) is ID-dependent due to ID function call and may cause performance degradation [FPGA-ID-dependent-backward-branch]
    accumulator++;
  }

  int j = 0;
  while (j < get_local_id(0)) {
    // CHECK-NOTES: :[[@LINE-1]]:14: warning: Backward branch (while loop) is ID-dependent due to ID function call and may cause performance degradation [FPGA-ID-dependent-backward-branch]
    accumulator++;
  }

  do {
    accumulator++;
  } while (j < get_local_id(0));
  // CHECK-NOTES: :[[@LINE-1]]:16: warning: Backward branch (do loop) is ID-dependent due to ID function call and may cause performance degradation [FPGA-ID-dependent-backward-branch]
  
  for (int i = 0; i < tid; i++) {
    // CHECK-NOTES: :[[@LINE-1]]:23: warning: Backward branch (for loop) is ID-dependent due to variable reference to 'tid' and may cause performance degradation [FPGA-ID-dependent-backward-branch]
    accumulator++;
  }

  while (j < tid) {
    // CHECK-NOTES: :[[@LINE-1]]:14: warning: Backward branch (while loop) is ID-dependent due to variable reference to 'tid' and may cause performance degradation [FPGA-ID-dependent-backward-branch]
    accumulator++;
  }

  do {
    accumulator++;
  } while (j < tid);
  // CHECK-NOTES: :[[@LINE-1]]:16: warning: Backward branch (do loop) is ID-dependent due to variable reference to 'tid' and may cause performance degradation [FPGA-ID-dependent-backward-branch]
  
  for (int i = 0; i < Example.IDDepField; i++) {
    // CHECK-NOTES: :[[@LINE-1]]:23: warning: Backward branch (for loop) is ID-dependent due to member reference to 'IDDepField' and may cause performance degradation [FPGA-ID-dependent-backward-branch]
    accumulator++;
  }

  while (j < Example.IDDepField) {
    // CHECK-NOTES: :[[@LINE-1]]:14: warning: Backward branch (while loop) is ID-dependent due to member reference to 'IDDepField' and may cause performance degradation [FPGA-ID-dependent-backward-branch]
    accumulator++;
  }

  do {
    accumulator++;
  } while (j < tid);
  // CHECK-NOTES: :[[@LINE-1]]:16: warning: Backward branch (do loop) is ID-dependent due to member reference to 'IDDepField' and may cause performance degradation [FPGA-ID-dependent-backward-branch]
}

void success() {
  int accumulator = 0;

  for (int i = 0; i < 1000; i++) {
    if (i < get_local_id(0)) {
      accumulator++;
    }
  }
}
