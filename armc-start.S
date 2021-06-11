
//  Part of the Raspberry-Pi Bare Metal Tutorials
//  Copyright (c) 2013-2015, Brian Sidebotham
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.

//  Relocate to just below 32MB

#include "tube-defs.h"

.equ    STACK_SIZE,           0x00100000

.equ    C0_SVR_STACK,        0
.equ    C0_IRQ_STACK,        (STACK_SIZE*1)
.equ    C0_FIQ_STACK,        STACK_SIZE*2
.equ    C0_USER_STACK,       STACK_SIZE*4
.equ    C0_ABORT_STACK,      STACK_SIZE*5
.equ    C0_UNDEFINED_STACK,  STACK_SIZE*6

#if defined(RPI2) || defined(RPI3)
.equ    C1_SVR_STACK,        STACK_SIZE*7
.equ    C1_IRQ_STACK,        STACK_SIZE*8
.equ    C1_FIQ_STACK,        STACK_SIZE*9
.equ    C1_USER_STACK,       STACK_SIZE*10
.equ    C1_ABORT_STACK,      STACK_SIZE*11
.equ    C1_UNDEFINED_STACK,  STACK_SIZE*12
#endif

.equ    SCTLR_ENABLE_DATA_CACHE,        0x4
.equ    SCTLR_ENABLE_BRANCH_PREDICTION, 0x800
.equ    SCTLR_ENABLE_INSTRUCTION_CACHE, 0x1000

 OS_GetEnv    = 0x10
 OS_Exit      = 0x11

.section .init, "ax"

.global _start
.global _get_cpsr
.global _get_stack_pointer

// From the ARM ARM (Architecture Reference Manual). Make sure you get the
// ARMv5 documentation which includes the ARMv6 documentation which is the
// correct processor type for the Broadcom BCM2835. The ARMv6-M manuals
// available on the ARM website are for Cortex-M parts only and are very
// different.
//
// See ARM section A2.2 (Processor Modes)

.equ    CPSR_MODE_USER,         0x10

_start:

    // Zero bss section
        ldr     r1, =__bss_start__
        ldr     r2, =__bss_end__
        mov     r3, #0
bsslp:
        teq     r1, r2
        strne   r3, [r1], #4
        bne     bsslp
	
    // Initialise Stack
        mov r0, #CPSR_MODE_USER  
        msr cpsr_c, r0
        //ldr sp, =0x1000000

    // Get application space limit
        swi     OS_GetEnv
    
        // Environment RAM limit becomes stack base (rounded down to nearest 8)
        bic     r13, r1, #7    
            
    // call application entry
        bl __appentry
 
    // exit
        swi     OS_Exit    

.section ".text._get_stack_pointer"
_get_stack_pointer:
    mov     r0, sp
    mov     pc, lr

.section ".text._get_cpsr"
_get_cpsr:
    mrs     r0, cpsr
    mov     pc, lr
