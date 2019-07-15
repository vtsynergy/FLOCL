// RUN: %check_clang_tidy %s opencl-possibly-unreachable-barrier %t -- -header-filter=.* "--" --include opencl-c.h -cl-std=CL1.2 -c 

// FIXME: Add something that triggers the check here.
__kernel void errors() {
  int tid = get_local_id(0);
  int gid = get_global_id(0);
  int tx = tid + get_local_size(0)*gid;
  for (int i = tx; i < 256; i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_id(0); i++) {
    barrier(CLK_LOCAL_MEM_FENCE);
  }  
// CHECK-MESSAGES: :[[@LINE-5]]:5: warning: Barrier inside for loop may not be reachable due to reference to ID-dependent variable 'i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-6]]:20 [opencl-possibly-unreachable-barrier]
// CHECK-MESSAGES: :[[@LINE-3]]:5: warning: Barrier inside for loop may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-4]]:19 [opencl-possibly-unreachable-barrier]
  for (int i = tx; i < 256; i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_id(0); i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }  
// CHECK-MESSAGES: :[[@LINE-5]]:5: warning: Barrier inside for loop may not be reachable due to reference to ID-dependent variable 'i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-6]]:20 [opencl-possibly-unreachable-barrier]
// CHECK-MESSAGES: :[[@LINE-3]]:5: warning: Barrier inside for loop may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-4]]:19 [opencl-possibly-unreachable-barrier]
  int ie_i = tx;
  if (ie_i < 256) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:7 [opencl-possibly-unreachable-barrier]
  }
  //The below stub should emit for both conditionals
  if (ie_i < 512) {	
// CHECK-MESSAGES: :[[@LINE+2]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:7 [opencl-possibly-unreachable-barrier]
  } else if (ie_i < 1024) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:14 [opencl-possibly-unreachable-barrier]
  }
  if (ie_i < 1024) {
  } else {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-3]]:7 [opencl-possibly-unreachable-barrier]
  }
  if (ie_i < 512) {	
  } else if (get_local_id(0) < 1024) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Barrier inside if/else may not be reachable due to ID function call in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-2]]:14 [opencl-possibly-unreachable-barrier]
// The below has to be put here because they report out of order for some reason
// CHECK-MESSAGES: :[[@LINE-3]]:5: warning: Barrier inside if/else may not be reachable due to reference to ID-dependent variable 'ie_i' in condition at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-5]]:7 [opencl-possibly-unreachable-barrier]
//TODO Implement While, do/while, and switch tests
  }
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
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }
  for (int i = 0; i < get_local_size(0); i++) {
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
  }  
}
