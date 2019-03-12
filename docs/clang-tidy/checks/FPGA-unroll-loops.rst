.. title:: clang-tidy - FPGA-unroll-loops

FPGA-unroll-loops
=================

Checks for inner loops that have not been unrolled. Unrolling
these loops could improve the performance of OpenCL kernels.

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

