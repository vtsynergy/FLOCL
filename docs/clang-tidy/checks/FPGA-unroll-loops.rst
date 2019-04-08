.. title:: clang-tidy - FPGA-unroll-loops

FPGA-unroll-loops
=================

Checks for inner loops that have not been unrolled. Unrolling
these loops could improve the performance of OpenCL kernels.

Also checks for fully unrolled loops with a large number of 
iterations, or unknown loop bounds. These loops cannot be fully
unrolled, and should be partially unrolled.

As per the "Altera SDK for OpenCL Best Practices Guide".

.. code-block:: c++

   for (int i = 0; i < 10; i++) {  // ok: outer loops should not be unrolled
      int j = 0;
      do {  // warning: this inner do..while loop should be unrolled
         j++;
      } while (j < 15);

      int k = 0;
      #pragma unroll
      while (k < 20) {  // ok: this inner loop is already unrolled 
         k++;
      }
   }

   #pragma unroll
   for (int i = 0; i < 1000; ++i) {  // warning: this loop is too large and cannot be fully unrolled
      printf("%d", i);
   }

   #pragma unroll 5
   for (int i = 0; i < 1000; ++i) {  // ok: this loop is large, but is partially unrolled
      printf("%d", i);
   }

   std::vector<int> someVector (100, 0);
   int i = 0;
   #pragma unroll
   while (i < someVector.size()) {  // warning: this loop has unknown bounds and cannot be fully unrolled
      someVector[i]++;
   }

   #pragma unroll
   while (true) {  // warning: this loop has unknown bounds and cannot be fully unrolled
      printf("In loop");
   }

   #pragma unroll
   while (i < 5) {  // warning: this loop has unknown bounds and cannot be fully unrolled
      printf("In loop");
   }

   #pragma unroll 5
   while (i < someVector.size()) {  // ok: this loop has unknown bounds, but is partially unrolled
      someVector[i]++;
   }

