// RUN: %check_clang_tidy %s FPGA-unroll-loops %t -- -header-filter=.* "--" --include opencl-c.h -cl-std=CL1.2 -c

// Inner loops should be unrolled
__kernel void nested_simple_loops(__global int *A) {
    for (int i = 0; i < 1000; ++i) {
        for (int j = 0; j < 2000; ++j) { 
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[0] += i + j;
        }
    }

    for (int i = 0; i < 1000; ++i) {
        int j = 0;
        while (j < 2000) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[1] += i + j;
            j++;
        }
    }

    for (int i = 0; i < 1000; ++i) {
        int j = 0; 
        do {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[2] += i + j;
            j++;
        } while (j < 2000);
    }

    int i = 0;
    while (i < 1000) {
        for (int j = 0; j < 2000; ++j) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[3] += i + j;
        }
        i++;
    }

    i = 0;
    while (i < 1000) {
        int j = 0;
        while (j < 2000) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[4] += i + j;
            j++;
        }
        i++;
    }

    i = 0;
    while (i < 1000) {
        int j = 0;
        do {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[5] += i + j;
            j++;
        } while (j < 2000);
        i++;
    }

    i = 0;
    do {
        for (int j = 0; j < 2000; ++j) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[6] += i + j;
        }
        i++;
    } while (i < 1000);

    i = 0;
    do {
        int j = 0;
        while (j < 2000) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[7] += i + j;
            j++;
        }
        i++;
    } while (i < 1000);

    i = 0;
    do {
        int j = 0;
        do {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
            A[8] += i + j;
            j++;
        } while (j < 2000);
        i++;
    } while (i < 1000);

    for (int i = 0; i < 100; ++i) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
        printf("Hello");
    }

    i = 0;
    while (i < 100) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
        i++;
    }

    i = 0;
    do {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: The performance of the kernel could be improved by unrolling this loop with a #pragma unroll directive [FPGA-unroll-loops]
        i++;
    } while (i < 100);
}

// These loops are all correctly unrolled
__kernel void unrolled_nested_simple_loops(__global int *A) {
    for (int i = 0; i < 1000; ++i) {
        #pragma unroll
        for (int j = 0; j < 2000; ++j) {
            A[0] += i + j;
        }
    }

    for (int i = 0; i < 1000; ++i) {
        int j = 0;
        #pragma unroll
        while (j < 2000) {
            A[1] += i + j;
            j++;
        }
    }

    for (int i = 0; i < 1000; ++i) {
        int j = 0; 
        #pragma unroll
        do {
            A[2] += i + j;
            j++;
        } while (j < 2000);
    }

    int i = 0;
    while (i < 1000) {
        #pragma unroll
        for (int j = 0; j < 2000; ++j) {
            A[3] += i + j;
        }
        i++;
    }

    i = 0;
    while (i < 1000) {
        int j = 0;
        #pragma unroll
        while (j < 2000) {
            A[4] += i + j;
            j++;
        }
        i++;
    }

    i = 0;
    while (i < 1000) {
        int j = 0;
        #pragma unroll
        do {
            A[5] += i + j;
            j++;
        } while (j < 2000);
        i++;
    }

    i = 0;
    do {
        #pragma unroll
        for (int j = 0; j < 2000; ++j) {
            A[6] += i + j;
        }
        i++;
    } while (i < 1000);

    i = 0;
    do {
        int j = 0;
        #pragma unroll
        while (j < 2000) {
            A[7] += i + j;
            j++;
        }
        i++;
    } while (i < 1000);

    i = 0;
    do {
        int j = 0;
        #pragma unroll
        do {
            A[8] += i + j;
            j++;
        } while (j < 2000);
        i++;
    } while (i < 1000);
}
// There are no fix-its for this check
