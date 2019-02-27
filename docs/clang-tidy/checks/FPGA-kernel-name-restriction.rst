.. title:: clang-tidy - FPGA-kernel-name-restriction

FPGA-kernel-name-restriction
============================

Finds kernel files whose filename is "kernel.cl", "Verilog.cl",
or "VHDL.cl".

Such kernel file names cause the offline compiler to generate intermediate 
design files that have the same names as certain internal files, which 
leads to a compilation error.

As per the "Guidelines for Naming the Kernel" section in the "Intel FPGA SDK 
for OpenCL Pro Edition: Programming Guide."
