// RUN: %check_clang_tidy %s fpga-single-work-item-barrier %t -- -header-filter=.* "--" --include opencl-c.h -cl-std=CL1.2 -c 

void __kernel error(__global int * foo, int size) {
for (int j = 0; j < 256; j++) {
	for (int i = 256; i < size; i+= 256) {
		foo[j] += foo[j+i];
	}
}
work_group_barrier(CLK_GLOBAL_MEM_FENCE);
// CHECK-MESSAGES: :[[@LINE-7]]:15: warning: Kernel function 'error' does not call get_global_id or get_local_id and will be treated as single-work-item.{{[[:space:]]}}Barrier call at {{(\/)?([^\/\0]+(\/)?)+}}:[[@LINE-1]]:1 may error out [fpga-single-work-item-barrier]
for (int i = 1; i < 256; i++) {
	foo[0] += foo[i];
}

}

void __kernel success(__global int * foo, int size) {
int tid = get_global_id(0);
for (int i = get_global_size(0); i < size; i+= get_global_size(0)) {
	foo[tid] += foo[tid +i];
}
work_group_barrier(CLK_GLOBAL_MEM_FENCE);
if (tid = 0) {
	for (int i = 1; i < get_global_size(0); i++) {
		foo[0] += foo[i];
	}
}
}
