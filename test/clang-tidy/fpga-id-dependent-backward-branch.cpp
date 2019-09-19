// RUN: %check_clang_tidy %s fpga-id-dependent-backward-branch %t -- -header-filter=.* "--" -cl-std=CL1.2 -c --include opencl-c.h

typedef struct ExampleStruct {
  int IDDepField;
} ExampleStruct;

void error() {
  // ==== Assignments ====
  int ThreadID = get_local_id(0); 
// CHECK-NOTES: :[[@LINE-1]]:3: warning: assignment of ID-dependent variable 'ThreadID' [fpga-id-dependent-backward-branch]

  ExampleStruct Example;
  Example.IDDepField = get_local_id(0);
// CHECK-NOTES: :[[@LINE-1]]:3: warning: assignment of ID-dependent field 'IDDepField' [fpga-id-dependent-backward-branch]

  // ==== Inferred Assignments ====
  int ThreadID2 = ThreadID * get_local_size(0);
// CHECK-NOTES: :[[@LINE-1]]:12: warning: Inferred assignment of ID-dependent value from ID-dependent variable 'ThreadID' [fpga-id-dependent-backward-branch]
  
  int ThreadID3 = Example.IDDepField;  // OK: not used in any loops

  ExampleStruct Example2 = {
    ThreadID * 2
  };

  // ==== Conditional Expressions ====
  int accumulator = 0;
  for (int i = 0; i < get_local_id(0); i++) {
    // CHECK-NOTES: :[[@LINE-1]]:23: warning: Backward branch (for loop) is ID-dependent due to ID function call and may cause performance degradation [fpga-id-dependent-backward-branch]
    accumulator++;
  }

  int j = 0;
  while (j < get_local_id(0)) {
    // CHECK-NOTES: :[[@LINE-1]]:14: warning: Backward branch (while loop) is ID-dependent due to ID function call and may cause performance degradation [fpga-id-dependent-backward-branch]
    accumulator++;
  }

  do {
    accumulator++;
  } while (j < get_local_id(0));
  // CHECK-NOTES: :[[@LINE-1]]:16: warning: Backward branch (do loop) is ID-dependent due to ID function call and may cause performance degradation [fpga-id-dependent-backward-branch]
  
  for (int i = 0; i < ThreadID2; i++) {
    // CHECK-NOTES: :[[@LINE-1]]:23: warning: Backward branch (for loop) is ID-dependent due to member reference to 'ThreadID2' and may cause performance degradation [fpga-id-dependent-backward-branch]
    accumulator++;
  }

  while (j < ThreadID) {
    // CHECK-NOTES: :[[@LINE-1]]:14: warning: Backward branch (while loop) is ID-dependent due to variable reference to 'ThreadID' and may cause performance degradation [fpga-id-dependent-backward-branch]
    accumulator++;
  }

  do {
    accumulator++;
  } while (j < ThreadID);
  // CHECK-NOTES: :[[@LINE-1]]:16: warning: Backward branch (do loop) is ID-dependent due to variable reference to 'ThreadID' and may cause performance degradation [fpga-id-dependent-backward-branch]
  
  for (int i = 0; i < Example.IDDepField; i++) {
    // CHECK-NOTES: :[[@LINE-1]]:23: warning: Backward branch (for loop) is ID-dependent due to member reference to 'IDDepField' and may cause performance degradation [fpga-id-dependent-backward-branch]
    accumulator++;
  }

  while (j < Example.IDDepField) {
    // CHECK-NOTES: :[[@LINE-1]]:14: warning: Backward branch (while loop) is ID-dependent due to member reference to 'IDDepField' and may cause performance degradation [fpga-id-dependent-backward-branch]
    accumulator++;
  }

  do {
    accumulator++;
  } while (j < Example.IDDepField);
  // CHECK-NOTES: :[[@LINE-1]]:16: warning: Backward branch (do loop) is ID-dependent due to member reference to 'IDDepField' and may cause performance degradation [fpga-id-dependent-backward-branch]
}

void success() {
  int accumulator = 0;

  for (int i = 0; i < 1000; i++) {
    if (i < get_local_id(0)) {
      accumulator++;
    }
  }
}
