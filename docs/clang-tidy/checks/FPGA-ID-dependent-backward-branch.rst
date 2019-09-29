.. title:: clang-tidy - FPGA-ID-dependent-backward-branch

FPGA-ID-dependent-backward-branch
=================================

Finds ID-dependent variables and fields that are used within loops. This causes
branches to occur inside the loops, and thus leads to performance degradation.

Based on the `Altera SDK for OpenCL: Best Practices Guide 
<https://www.altera.com/en_US/pdfs/literature/hb/opencl-sdk/aocl_optimization_guide.pdf>`_.

