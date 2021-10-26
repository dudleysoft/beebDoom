
        .title  ARM coprocessor C library support

        // For use with the Yagarto GCC tool chain 4.4.1
        // (C)2010 SPROW

        .global  _swi
        .global  _swix
        .global  _exit
        .global	 _kernel_swi
        .global _OS_Byte
        .global _OS_Word
        .global _OS_NewLine
        .global _OS_Args
        .global _OS_Find
        .global _OS_WriteC
        .global _OS_GBPB
        .global _OS_ReadC
        .global _OS_WriteI
        .global _OS_ReadLine
        .global _OS_CLI
        .global _OS_MMUControl
        .global _OS_ReadVduVariables
        .global _VDU
        
        .extern  main


        .section armtubeswis, "ax"
        .code 32

        OS_Exit                 = 0x11
        XOS_Exit                = 0x20011
        OS_CallASWIR12          = 0x71
        XOS_CallASWIR12         = 0x20071
        OS_ChangeEnvironment    = 0x40
        XOS_ChangeEnvironment   = 0x20040
        OS_Byte                 = 0x06		    
        OS_Word                 = 0x07
        OS_NewLine              = 0x03
        OS_Args                 = 0x09
        OS_Find                 = 0x0D
        OS_WriteC               = 0x00
        OS_GBPB                 = 0x0C
        OS_ReadC                = 0x04
        OS_WriteI               = 0x100
        OS_ReadLine             = 0x0E
        OS_CLI                  = 0x05
        OS_MMUControl           = 0x6b
        OS_ReadVduVariables     = 0x31


        USRStackSize            = 32768
        V_bit                   = 1 << 28
        C_bit                   = 1 << 29
        Z_bit                   = 1 << 30
        N_bit                   = 1 << 31

    // _kernel_oserror *_kernel_swi(int no, _kernel_swi_regs *in,
    //                              _kernel_swi_regs *out);
    
_kernel_swi:
        STMFD	sp!, {a3, v1-v6, lr}
        BIC	    ip, a1, #0x80000000
        TEQ	    ip, a1
        ORREQ	ip, ip, #0x20000		@ X bit
        LDMIA	a2, {a1-v6}
        SWI	    XOS_CallASWIR12
        LDR	    ip, [sp, #0]
        STMIA	ip, {a1-v6}
        MOVVC	a1, #0
        LDMVCFD	sp!, {a3, v1-v6, pc}
        MOV	    v1, a1
        MOV	    a2, #1
        // BL	__ul_seterr
        MOV	    a1, v1
        LDMFD	sp!, {a3, v1-v6, pc}

_swix:
        // Fall through to _swi() function
        //ORR     r0, r0, #0x20000

_swi:
        // Push so stack contains all entry varargs in order
        STMFD   r13!, {r2-r3}

        // Registers used
        STMFD   r13!, {r4-r11, lr}
        MOV     lr, r1
        MOV     r12, r0

        // Select registers as requested by _IN()
        TST     lr, #0x200
        LDRNE   r9, [r13, #36+(9*4)]
        TST     lr, #0x100
        LDRNE   r8, [r13, #36+(8*4)]
        TST     lr, #0x80
        LDRNE   r7, [r13, #36+(7*4)]
        TST     lr, #0x40
        LDRNE   r6, [r13, #36+(6*4)]
        TST     lr, #0x20
        LDRNE   r5, [r13, #36+(5*4)]
        TST     lr, #0x10
        LDRNE   r4, [r13, #36+(4*4)]
        TST     lr, #0x8
        LDRNE   r3, [r13, #36+(3*4)]
        TST     lr, #0x4
        LDRNE   r2, [r13, #36+(2*4)]
        TST     lr, #0x2
        LDRNE   r1, [r13, #36+(1*4)]
        TST     lr, #0x1
        LDRNE   r0, [r13, #36+(0*4)]

        // Test for required SWIs 
        TEQ     r12, #OS_WriteC
        BEQ     _OS_WriteC
        TEQ	r12, #OS_Byte
        BEQ     _OS_Byte		    
        TEQ     r12, #OS_Word
        BEQ     _OS_Word
        TEQ     r12, #OS_NewLine
        BEQ     _OS_NewLine              
        TEQ	r12, #OS_Args
        BEQ     _OS_Args
        TEQ     r12, #OS_Find 
        BEQ     _OS_Find
        TEQ     r12, #OS_GBPB
        BEQ     _OS_GBPB
        TEQ     r12, #OS_ReadC
        BEQ     _OS_ReadC
        CMP     r12, #OS_ReadLine
        BEQ     _OS_ReadLine
        CMP     r12, #OS_CLI
        BEQ     _OS_CLI
        CMP     r12, #OS_MMUControl
        BEQ     _OS_MMUControl
        CMP     r12, #OS_ReadVduVariables
        BEQ     _OS_ReadVduVariables

        CMP     r12, #OS_WriteI
        BGE     _OS_WriteI

        // otherwise use callaswir12 (unimplemented so report num in R0)
 unswi: MOV     R0, R12 
        SWI     XOS_CallASWIR12         

        // Keep flags and X bit
swret:  MRS     r10, CPSR
        AND     r10, r10, #N_bit | Z_bit | C_bit | V_bit
        AND     r12, r12, #0x20000
        ORR     r12, r10, r12
        
        // Count how many bits were input to find the output address start
        ADD     r10, r13, #36
        MOV     r11, #0x200
b10:
        TST     r11, lr
        ADDNE   r10, r10, #4
        MOVS    r11, r11, LSR#1
        BNE     b10

        // Skip when no output words are expected
        MOVS    r11, lr, LSR#22
        BEQ     b20

        // Save registers as requested by _OUT()
        TST     lr, #0x80000000
        LDRNE   r11, [r10], #4
        STRNE   r0, [r11]
        TST     lr, #0x40000000
        LDRNE   r11, [r10], #4
        STRNE   r1, [r11]
        TST     lr, #0x20000000
        LDRNE   r11, [r10], #4
        STRNE   r2, [r11]
        TST     lr, #0x10000000
        LDRNE   r11, [r10], #4
        STRNE   r3, [r11]
        TST     lr, #0x8000000
        LDRNE   r11, [r10], #4
        STRNE   r4, [r11]
        TST     lr, #0x4000000
        LDRNE   r11, [r10], #4
        STRNE   r5, [r11]
        TST     lr, #0x2000000
        LDRNE   r11, [r10], #4
        STRNE   r6, [r11]
        TST     lr, #0x1000000
        LDRNE   r11, [r10], #4
        STRNE   r7, [r11]
        TST     lr, #0x800000
        LDRNE   r11, [r10], #4
        STRNE   r8, [r11]
        TST     lr, #0x400000
        LDRNE   r11, [r10], #4
        STRNE   r9, [r11]
b20:
        // Check if output flags were requested too
        TST     lr, #0x200000
        LDRNE   r11, [r10]
        STRNE   r12, [r11]

        // Error returns are only 0 or error pointer
        TST     r12, #0x20000
        BEQ     b30
        TST     r12, #V_bit
        MOVEQ   r0, #0
        B       b40
b30:
        // Check the return register number
        MOV     lr, lr, LSR#16
        BIC     lr, lr, #15
        TEQ     lr, #15
        MOVEQ   r0, r12
        STMNEFD r13!, {r0-r9}
        MOVNE   lr, lr, LSL#2
        LDRNE   r0, [r13, lr]
        ADDNE   r13, r13, #4*10
b40:
        // Back to C
        LDMFD   r13!, {r4-r11, lr}
        ADD     r13, r13, #8
        MOV     pc, lr


_exit:
        // Exit
        SWI     OS_Exit

_OS_WriteC:
	SWI OS_WriteC
	B swret
_OS_Byte:
	SWI OS_Byte
	B swret
_OS_Word:
	SWI OS_Word
	B swret
_OS_NewLine:
	SWI OS_NewLine
	B swret
_OS_Args:
	SWI OS_Args
	B swret
_OS_Find:
	SWI OS_Find
	B swret	
_OS_GBPB:
	SWI OS_GBPB
	B swret
_OS_ReadC:
	SWI OS_ReadC
	B swret
_OS_ReadLine:
     SWI OS_ReadLine
     B swret
_OS_CLI:
     SWI OS_CLI
     B swret
_OS_WriteI:
     CMP r12, #0x200
     BGE unswi
     SUB r0,r12,#0x100
	SWI OS_WriteC
        B swret

_OS_ReadVduVariables:
     SWI OS_ReadVduVariables
     B swret

_VDU:
     SWI OS_WriteC
     MOV pc, lr

_OS_MMUControl:
     SWI OS_MMUControl
     B swret