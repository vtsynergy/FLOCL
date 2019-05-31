// RUN: %check_clang_tidy %s FPGA-unroll-loops %t -- -config="{CheckOptions: [{key: "FPGA-unroll-loops.max_loop_iterations", value: 50}]}" -header-filter=.* "--" --include opencl-c.h -cl-std=CL1.2 -c

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
        for (int j = 0; j < 50; ++j) {
            A[0] += i + j;
        }
    }

    for (int i = 0; i < 1000; ++i) {
        int j = 0;
        #pragma unroll
        while (j < 50) {
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
        } while (j < 50);
    }

    int i = 0;
    while (i < 1000) {
        #pragma unroll
        for (int j = 0; j < 50; ++j) {
            A[3] += i + j;
        }
        i++;
    }

    i = 0;
    while (i < 1000) {
        int j = 0;
        #pragma unroll
        while (50 > j) {
            A[4] += i + j;
            j++;
        }
        i++;
    }

    i = 0;
    while (1000 > i) {
        int j = 0;
        #pragma unroll
        do {
            A[5] += i + j;
            j++;
        } while (j < 50);
        i++;
    }

    i = 0;
    do {
        #pragma unroll
        for (int j = 0; j < 50; ++j) {
            A[6] += i + j;
        }
        i++;
    } while (i < 1000);

    i = 0;
    do {
        int j = 0;
        #pragma unroll
        while (j < 50) {
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
        } while (j < 50);
        i++;
    } while (i < 1000);
}

__kernel void unrolled_nested_simple_loops_large_num_iterations(__global int *A) {
    for (int i = 0; i < 1000; ++i) {
        #pragma unroll
        for (int j = 0; j < 51; ++j) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
            A[0] += i + j;
        }
    }

    for (int i = 0; i < 1000; ++i) {
        int j = 0;
        #pragma unroll
        while (j < 51) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
            A[1] += i + j;
            j++;
        }
    }

    for (int i = 0; i < 1000; ++i) {
        int j = 0; 
        #pragma unroll
        do {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
            A[2] += i + j;
            j++;
        } while (j < 51);
    }

    int i = 0;
    while (i < 1000) {
        #pragma unroll
        for (int j = 0; j < 51; ++j) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
            A[3] += i + j;
        }
        i++;
    }

    i = 0;
    while (i < 1000) {
        int j = 0;
        #pragma unroll
        while (51 > j) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
            A[4] += i + j;
            j++;
        }
        i++;
    }

    i = 0;
    while (1000 > i) {
        int j = 0;
        #pragma unroll
        do {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
            A[5] += i + j;
            j++;
        } while (j < 51);
        i++;
    }

    i = 0;
    do {
        #pragma unroll
        for (int j = 0; j < 51; ++j) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
            A[6] += i + j;
        }
        i++;
    } while (i < 1000);

    i = 0;
    do {
        int j = 0;
        #pragma unroll
        while (j < 51) {
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
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
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: This loop likely has a large number of iterations and thus cannot be fully unrolled. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
            A[8] += i + j;
            j++;
        } while (j < 51);
        i++;
    } while (i < 1000);
}

__kernel void fully_unrolled_unknown_bounds(int vectorSize) {
    int someVector[101];

    // There is no loop condition
    #pragma unroll
    for (;;) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        printf("%d", someVector);
    }

    // Infinite loop
    #pragma unroll
    while (true) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        printf("%d", someVector);
    }

    #pragma unroll
    do {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        printf("%d", someVector);
    } while (true);

    #pragma unroll
    while (1 < 5) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        printf("%d", someVector);
    }

    #pragma unroll
    do {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        printf("%d", someVector);
    } while (1 < 5);

    #pragma unroll
    for (int i = 0; 1 < 5; ++i) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        someVector[i]++;
    }

    // Both sides are value-dependent
    #pragma unroll
    for (int i = 0; i < vectorSize; ++i) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        printf("%d", someVector);
    }

    int j = 0;
    #pragma unroll
    while (j < vectorSize) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        printf("%d", someVector);
    }
    
    #pragma unroll
    do {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Full unrolling was requested, but loop bounds are not known. To partially unroll this loop, use the #pragma unroll <num> directive [FPGA-unroll-loops]
        printf("%d", someVector);
    } while (j < vectorSize);
}
// There are no fix-its for this check
