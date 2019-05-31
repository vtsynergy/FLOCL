.. title:: clang-tidy - OpenCL-recursion-not-supported

OpenCL-recursion-not-supported
==============================

Finds recursive function calls and flags them as compiler errors, since 
recursion is not supported in OpenCL as per the official OpenCL restrictions 
list.

The depth, in terms of number of functions through which the lint check will 
attempt to search for a recursive function call is limited by the 
``MaxRecursionDepth`` option parameter.

Examples:

.. code-block:: c++

  int fibonacci(int num) {
    if (num < 2) {
      return 1;
    }
    // error: fibonacci calls itself
    return fibonacci(num-2) + fibonacci(num-1);
  }

  void recursiveA() {
    recursiveB();
  }

  void recursiveB() {
    // error: recursiveB calls recursiveA, and recursiveA calls recursiveB
    recursiveA();
  }
