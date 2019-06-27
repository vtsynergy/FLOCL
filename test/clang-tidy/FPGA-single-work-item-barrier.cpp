// RUN: %check_clang_tidy -check-suffix=OLD %s FPGA-single-work-item-barrier %t -- -header-filter=.* "--" -cl-std=CL1.2 -c --include opencl-c.h -DOLD
// RUN: %check_clang_tidy -check-suffix=NEW %s FPGA-single-work-item-barrier %t -- -header-filter=.* "--" -cl-std=CL2.0 -c --include opencl-c.h -DNEW

#ifdef OLD
void __kernel error_barrier_no_id(__global int * foo, int size) {
  for (int j = 0; j < 256; j++) {
	for (int i = 256; i < size; i+= 256) {
      foo[j] += foo[j+i];
    }
  }
  barrier(CLK_GLOBAL_MEM_FENCE);
  // CHECK-MESSAGES-OLD: :[[@LINE-7]]:15: warning: Kernel function 'error_barrier_no_id' does not call get_global_id or get_local_id and will be treated as single-work-item.{{[[:space:]]}}Barrier call at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:3 may error out [FPGA-single-work-item-barrier]
  for (int i = 1; i < 256; i++) {
	foo[0] += foo[i];
  }
}

void __kernel success_barrier_global_id(__global int * foo, int size) {
  barrier(CLK_GLOBAL_MEM_FENCE);
  int tid = get_global_id(0);
}

void __kernel success_barrier_local_id(__global int * foo, int size) {
  barrier(CLK_GLOBAL_MEM_FENCE);
  int tid = get_local_id(0);
}

void __kernel success_barrier_both_ids(__global int * foo, int size) {
  barrier(CLK_GLOBAL_MEM_FENCE);
  int gid = get_global_id(0);
  int lid = get_local_id(0);
}

void success_nokernel_barrier_no_id(__global int * foo, int size) {
  for (int j = 0; j < 256; j++) {
	for (int i = 256; i < size; i+= 256) {
      foo[j] += foo[j+i];
    }
  }
  barrier(CLK_GLOBAL_MEM_FENCE);
  for (int i = 1; i < 256; i++) {
	foo[0] += foo[i];
  }
}

void success_nokernel_barrier_global_id(__global int * foo, int size) {
  barrier(CLK_GLOBAL_MEM_FENCE);
  int tid = get_global_id(0);
}

void success_nokernel_barrier_local_id(__global int * foo, int size) {
  barrier(CLK_GLOBAL_MEM_FENCE);
  int tid = get_local_id(0);
}

void success_nokernel_barrier_both_ids(__global int * foo, int size) {
  barrier(CLK_GLOBAL_MEM_FENCE);
  int gid = get_global_id(0);
  int lid = get_local_id(0);
}
#endif

#ifdef NEW
void __kernel error_barrier_no_id(__global int * foo, int size) {
  for (int j = 0; j < 256; j++) {
	for (int i = 256; i < size; i+= 256) {
      foo[j] += foo[j+i];
    }
  }
  work_group_barrier(CLK_GLOBAL_MEM_FENCE);
  // CHECK-MESSAGES-NEW: :[[@LINE-7]]:15: warning: Kernel function 'error_barrier_no_id' does not call get_global_id or get_local_id and will be treated as single-work-item.{{[[:space:]]}}Barrier call at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:3 may error out [FPGA-single-work-item-barrier]
  for (int i = 1; i < 256; i++) {
	foo[0] += foo[i];
  }
}

void __kernel success_barrier_global_id(__global int * foo, int size) {
  work_group_barrier(CLK_GLOBAL_MEM_FENCE);
  int tid = get_global_id(0);
}

void __kernel success_barrier_local_id(__global int * foo, int size) {
  work_group_barrier(CLK_GLOBAL_MEM_FENCE);
  int tid = get_local_id(0);
}

void __kernel success_barrier_both_ids(__global int * foo, int size) {
  work_group_barrier(CLK_GLOBAL_MEM_FENCE);
  int gid = get_global_id(0);
  int lid = get_local_id(0);
}

void success_nokernel_barrier_no_id(__global int * foo, int size) {
  for (int j = 0; j < 256; j++) {
	for (int i = 256; i < size; i+= 256) {
      foo[j] += foo[j+i];
    }
  }
  work_group_barrier(CLK_GLOBAL_MEM_FENCE);
  for (int i = 1; i < 256; i++) {
	foo[0] += foo[i];
  }
}

void success_nokernel_barrier_global_id(__global int * foo, int size) {
  work_group_barrier(CLK_GLOBAL_MEM_FENCE);
  int tid = get_global_id(0);
}

void success_nokernel_barrier_local_id(__global int * foo, int size) {
  work_group_barrier(CLK_GLOBAL_MEM_FENCE);
  int tid = get_local_id(0);
}

void success_nokernel_barrier_both_ids(__global int * foo, int size) {
  work_group_barrier(CLK_GLOBAL_MEM_FENCE);
  int gid = get_global_id(0);
  int lid = get_local_id(0);
}
#endif
