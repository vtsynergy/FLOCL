// RUN: %check_clang_tidy %s fpga-kernel-name-restriction %t -- -- -I%S/Inputs/fpga-kernel-name-restriction

// These are the banned kernel filenames, and should trigger warnings
#include "kernel.cl"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]
#include "Verilog.cl"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]
#include "VHDL.cl"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]

// The warning should be triggered regardless of capitalization
#include "KERNEL.cl"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]
#include "vERILOG.cl"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]
#include "vhdl.CL"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]

// The warning should be triggered if the names are within a directory
#include "some/dir/kernel.cl"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]
#include "somedir/verilog.cl"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]
#include "otherdir/vhdl.cl"
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: The imported kernel source file is named 'kernel.cl','Verilog.cl', or 'VHDL.cl', which could cause compilation errors. [fpga-kernel-name-restriction]

// There are no FIX-ITs for the fpga-kernel-name-restriction lint check

// The following include directives shouldn't trigger the warning
#include "otherthing.cl"
#include "thing.h"

// It doesn't make sense to have kernel.h, verilog.h, or vhdl.h as filenames without the corresponding .cl files,
// but the Altera Programming Guide doesn't explicitly forbid it
#include "kernel.h"
#include "verilog.h"
#include "vhdl.h"

// The files can still have the forbidden names in them, so long as they're not the entire file name
#include "some_kernel.cl"
#include "other_Verilog.cl"
#include "vhdl_number_two.cl"
