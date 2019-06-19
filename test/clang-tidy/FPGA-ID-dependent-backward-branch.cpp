// RUN: %check_clang_tidy %s FPGA-ID-dependent-backward-branch %t -- -header-filter=.* "--" -cl-std=CL1.2 -c --include opencl-c.h 

// Mock up essential functions
/*int get_local_id(int a) {
  return a;
}

int get_local_size(int a) {
  return a;  
}*/

void error() {
  int tid = get_local_id(0);
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: assignment of ID-dependent variable 'tid' declared at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:3 [FPGA-ID-dependent-backward-branch]
  int tx = tid*get_local_size(0);
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: Inferred assignment of ID-dependent value from ID-dependent variable 'tid' [FPGA-ID-dependent-backward-branch]
  for(int i = 0; i < 256; i++) {
    if (i < tid) {
      // XCHECK-MESSAGES: :[[@LINE-1]]:9: warning: Conditional inside loop is ID-dependent due to variable reference to 'tid' [FPGA-ID-dependent-backward-branch]
    } 
    if (i < get_global_id(0)) {
      // XCHECK-MESSAGES: :[[@LINE-1]]:13: warning: Conditional inside loop is ID-dependent due to ID function call [FPGA-ID-dependent-backward-branch]
    }
  }
  int j = 0;
  while (j < 256) {
    if (j < tid) {
      // XCHECK-MESSAGES: :[[@LINE-1]]:9: warning: Conditional inside loop is ID-dependent due to variable reference to 'tid' [FPGA-ID-dependent-backward-branch]
    }
    if (j < get_global_id(0)) {
      // XCHECK-MESSAGES: :[[@LINE-1]]:13: warning: Conditional inside loop is ID-dependent due to ID function call [FPGA-ID-dependent-backward-branch]
    }
    j++;
  }
  do {
    if (j < tid) {
      // XCHECK-MESSAGES: :[[@LINE-1]]:9: warning: Conditional inside loop is ID-dependent due to variable reference to 'tid' [FPGA-ID-dependent-backward-branch]
    }
    if (j < get_global_id(0)) {
      // XCHECK-MESSAGES: :[[@LINE-1]]:13: warning: Conditional inside loop is ID-dependent due to ID function call [FPGA-ID-dependent-backward-branch]
    }
    j++;
  } while (j < 256);
}

void success() {

}
