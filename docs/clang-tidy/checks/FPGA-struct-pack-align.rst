.. title:: clang-tidy - FPGA-struct-pack-align

FPGA-struct-pack-align
======================

Finds structs that are inefficiently packed or aligned, and recommends
packing and/or aligning of said structs as needed. 

Structs that are not packed take up more space than they should, and accessing 
structs that are not well aligned is inefficient.

Based on the "Altera SDK for OpenCL: Best Practices Guide".

.. code-block:: c++

  // The following struct is originally aligned to 4 bytes, and thus takes up
  // 12 bytes of memory instead of 10. Packing the struct will make it use
  // only 10 bytes of memory, and aligning it to 16 bytes will make it 
  // efficient to access. 
  struct example {
    char a;    // 1 byte
    double b;  // 8 bytes
    char c;    // 1 byte
  };

  // The following struct is arranged in such a way that packing is not needed.
  // However, it is aligned to 4 bytes instead of 8, and thus needs to be 
  // explicitly aligned.
  struct implicitly_packed_example {
    char a;  // 1 byte
    char b;  // 1 byte
    char c;  // 1 byte
    char d;  // 1 byte
    int e;   // 4 bytes
  };

  // The following struct is explicitly aligned and packed. 
  struct good_example {
    char a;    // 1 byte
    double b;  // 8 bytes
    char c;    // 1 byte
  } __attribute((packed)) __attribute((aligned(16));

  // Explicitly aligning a struct to a bad value will result in a warning
  struct badly_aligned_example {
    char a;    // 1 byte
    double b;  // 8 bytes
    char c;    // 1 byte
  } __attribute((packed)) __attribute((aligned(32)));
