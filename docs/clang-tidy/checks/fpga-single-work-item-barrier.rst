.. title:: clang-tidy - fpga-single-work-item-barrier

fpga-single-work-item-barrier
=============================

Finds OpenCL kernel functions that call a barrier function but do not call
an ID function.

These kernel functions will be treated as single work-item kernels, which
could be inefficient or lead to errors.

The version of the Altera Offline Compiler can be specified using the
``AOCVersion`` option paramter.

Examples:

.. code-block:: c++
  
  // error: function calls barrier but does not call an ID function
  void __kernel barrier_no_id(__global int * foo, int size) {
    for (int i = 0; i < 100; i++) {
      foo[i] += 5;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);
  }

  // ok: function calls barrier and an ID function
  void __kernel barrier_with_id(__global int * foo, int size) {
    for (int i = 0; i < 100; i++) {
      int tid = get_global_id(0);
      foo[tid] += 5;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);
  }
