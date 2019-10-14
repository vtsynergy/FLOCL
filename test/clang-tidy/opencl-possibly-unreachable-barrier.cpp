// RUN: %check_clang_tidy -check-suffix=OLD %s opencl-possibly-unreachable-barrier %t -- -header-filter=.* "--" -cl-std=CL1.2 -c --include opencl-c.h -DOLD
// RUN: %check_clang_tidy -check-suffix=NEW %s opencl-possibly-unreachable-barrier %t -- -header-filter=.* "--" -cl-std=CL2.0 -c --include opencl-c.h -DNEW

#ifdef OLD
__kernel void errors() {
  int tid = get_local_id(0);
  int gid = get_global_id(0);
  int tx = tid + get_local_size(0)*gid;
  for (int i = tx; i < 256; i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES-OLD: :[[@LINE-1]]:5: warning: Barrier inside for loop may not be reachable due to reference to ID-dependent variable 'i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:20 [opencl-possibly-unreachable-barrier]
  }
  for (int i = 0; i < get_local_id(0); i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES-OLD: :[[@LINE-1]]:5: warning: Barrier inside for loop may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:19 [opencl-possibly-unreachable-barrier]
  }  
  for (int i = tx; i < 256; i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_id(0); i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
  }  
// CHECK-MESSAGES-OLD: :[[@LINE-5]]:5: warning: Barrier inside for loop may not be reachable due to reference to ID-dependent variable 'i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-6]]:20 [opencl-possibly-unreachable-barrier]
// CHECK-MESSAGES-OLD: :[[@LINE-3]]:5: warning: Barrier inside for loop may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-4]]:19 [opencl-possibly-unreachable-barrier]
  int ie_i = tx;
  if (ie_i < 256) {
    barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES-OLD: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:7 [opencl-possibly-unreachable-barrier]
  }
  //The below stub should emit for both conditionals
  if (ie_i < 512) {	
// CHECK-MESSAGES-OLD: :[[@LINE+2]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:7 [opencl-possibly-unreachable-barrier]
  } else if (ie_i < 1024) {
    barrier(CLK_LOCAL_MEM_FENCE);
// XCHECK-MESSAGES-OLD: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:14 [opencl-possibly-unreachable-barrier]
  }
  if (ie_i < 1024) {
// CHECK-MESSAGES-OLD: :[[@LINE+2]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:7 [opencl-possibly-unreachable-barrier]
  } else {
    barrier(CLK_LOCAL_MEM_FENCE);
// XCHECK-MESSAGES-OLD: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-3]]:7 [opencl-possibly-unreachable-barrier]
  }
  if (ie_i < 512) {	
// CHECK-MESSAGES-OLD: :[[@LINE+3]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:7 [opencl-possibly-unreachable-barrier]
// XCHECK-MESSAGES-OLD: :[[@LINE+2]]:5: warning: Barrier inside if/else may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:14 [opencl-possibly-unreachable-barrier]
  } else if (get_local_id(0) < 1024) {
    barrier(CLK_LOCAL_MEM_FENCE);
// The below has to be put here because they report out of order for some reason
//TODO Implement While, do/while, and switch tests
  }

  // TODO: if statement that calls get_local_id() in CondExpr
}

__kernel void correct() {
  int tid = get_local_id(0);
  int gid = get_global_id(0);
  int tx = tid + get_local_size(0)*gid;
  for (int i = 0; i < 256; i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_size(0); i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
  }  
  for (int i = 0; i < 256; i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_size(0); i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
  }  
}
#endif

#ifdef NEW
__kernel void errors() {
  int tid = get_local_id(0);
  int gid = get_global_id(0);
  int tx = tid + get_local_size(0)*gid;
  for (int i = tx; i < 256; i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES-NEW: :[[@LINE-1]]:5: warning: Barrier inside for loop may not be reachable due to reference to ID-dependent variable 'i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:20 [opencl-possibly-unreachable-barrier]
  }
  for (int i = 0; i < get_local_id(0); i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES-NEW: :[[@LINE-1]]:5: warning: Barrier inside for loop may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:19 [opencl-possibly-unreachable-barrier]
  }  
  for (int i = tx; i < 256; i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_id(0); i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }  
// CHECK-MESSAGES-NEW: :[[@LINE-5]]:5: warning: Barrier inside for loop may not be reachable due to reference to ID-dependent variable 'i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-6]]:20 [opencl-possibly-unreachable-barrier]
// CHECK-MESSAGES-NEW: :[[@LINE-3]]:5: warning: Barrier inside for loop may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-4]]:19 [opencl-possibly-unreachable-barrier]
  int ie_i = tx;
  if (ie_i < 256) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES-NEW: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:7 [opencl-possibly-unreachable-barrier]
  }
  //The below stub should emit for both conditionals
  if (ie_i < 512) {	
// CHECK-MESSAGES-NEW: :[[@LINE+2]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:7 [opencl-possibly-unreachable-barrier]
  } else if (ie_i < 1024) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// XCHECK-MESSAGES-NEW: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:14 [opencl-possibly-unreachable-barrier]
  }
  if (ie_i < 1024) {
// CHECK-MESSAGES-NEW: :[[@LINE+2]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:7 [opencl-possibly-unreachable-barrier]
  } else {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// XCHECK-MESSAGES-NEW: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-3]]:7 [opencl-possibly-unreachable-barrier]
  }
  if (ie_i < 512) {	
// CHECK-MESSAGES-NEW: :[[@LINE+3]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:7 [opencl-possibly-unreachable-barrier]
// XCHECK-MESSAGES-NEW: :[[@LINE+2]]:5: warning: Barrier inside if/else may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:14 [opencl-possibly-unreachable-barrier]
  } else if (get_local_id(0) < 1024) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// The below has to be put here because they report out of order for some reason
//TODO Implement While, do/while, and switch tests
  }

  // TODO: if statement that calls get_local_id() in CondExpr
}

__kernel void correct() {
  int tid = get_local_id(0);
  int gid = get_global_id(0);
  int tx = tid + get_local_size(0)*gid;
  for (int i = 0; i < 256; i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_size(0); i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }  
  for (int i = 0; i < 256; i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_size(0); i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }  
}
#endif
