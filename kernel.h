/* kernel.h

   For use with the GNU compilers and the SharedCLibrary.
   (c) Copyright 1997, Nick Burrett.  */

#ifndef __KERNEL_H
#define __KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

/* GCC has various useful declarations that can be made with the
   __attribute__ syntax.  Disable its use for other compilers.  */
#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x) /* Ignore.  */
#endif
#endif

typedef struct
{
  int errnum;
  char errmess[252];
} _kernel_oserror;

/* Stack management functions.  */

typedef struct stack_chunk
{
  unsigned long sc_mark; /* == 0xf60690ff */
  struct stack_chunk *sc_next, *sc_prev;
  unsigned long sc_size;
  int (*sc_deallocate)(void);
} _kernel_stack_chunk;

/* Return a pointer to the current stack chunk.  */
extern _kernel_stack_chunk *_kernel_current_stack_chunk (void);

typedef struct
{
  int r4, r5, r6, r7, r8, r9;
  int fp, sp, pc, sl;
  int f4[3], f5[3], f6[3], f7[3];
} _kernel_unwindblock;

/* Unwind the call stack one level. Return: >0 on success,
   0 if stack end has been reached, <0 any other failures.  */
extern int _kernel_unwind (_kernel_unwindblock *__inout, char **__language);

/* Program environment functions.  */

/* Return a pointer to the name of the function that contains the
   address 'pc' (or zero if no name can be found).  */
extern char *_kernel_procname (int __pc);

/* Return a pointer to the name of the language that the address
   'pc' lies within. Zero if the language is unknown. */
extern char *_kernel_language (int __pc);

/* Return a pointer to the command string used to run the program.  */
extern char *_kernel_command_string (void);

/* Set the return code to be used by _kernel_exit.  */
extern void _kernel_setreturncode (unsigned __code);

/* Call OS_Exit. Use the return code set by _kernel_setreturncode.  */
extern void _kernel_exit (int __value) __attribute__ ((__noreturn__));

/* Generates an external error.  */
extern void _kernel_raise_error (_kernel_oserror *);

/* Reset the InTrapHandler flag to prevent recursive traps.  */
extern void _kernel_exittraphandler (void);

/* Returns 6 for RISC OS.  */
extern int _kernel_hostos (void);

/* Return non-zero if floating point instructions are available.  */
extern int _kernel_fpavailable (void);

/* Return the last OS error since the last time _kernel_last_oserror
   was called. Return zero if no errors have occurred.  */
extern _kernel_oserror *_kernel_last_oserror (void);

/* Read the value of system variable 'name', placing the result in 'buffer'.  */
extern _kernel_oserror *_kernel_getenv (const char *__name,
					char *__buffer, unsigned __size);

/* Set the system variable 'name' with 'value'. If 'value == 0' then
   'name' is deleted.  */
extern _kernel_oserror *_kernel_setenv (const char *__name,
					const char *__value);

/* Return 1 if there has been an Escape since the previous call to
   _kernel_escape_seen.  */
extern int _kernel_escape_seen (void);

/* Enable IRQ interrupts.  This can only be executed within SVC mode.  */
extern void _kernel_irqs_on (void);

/* Disable IRQ interrupts.  This can only be executed within SVC mode.  */
extern void _kernel_irqs_off (void);

/* Return non-zero if IRQs are disabled.  */
extern int _kernel_irqs_disabled (void);

/* General utility functions.  */

typedef struct
{
  int r[10];
} _kernel_swi_regs;

/* Call the SWI specified by 'no'. 'in' points to a register block for SWI
   entry.  'out' points to a register block for SWI exit. The X bit is set
   by _kernel_swi unless bit 31 is set.  */
extern _kernel_oserror *_kernel_swi (int __no, _kernel_swi_regs *__in,
				     _kernel_swi_regs *__out);


/* Similar to _kernel_swi but the carry flag status is returned in
   'carry'.  */
extern _kernel_oserror *_kernel_swi_c (int __no, _kernel_swi_regs *__in,
				       _kernel_swi_regs *__out,
				       int *__carry);

/* Perform an OS_Byte operation.
   R1 is returned in the bottom byte, R2 in the second byte,
   if carry set, then third byte = 1.  */
extern int _kernel_osbyte (int __operation, int __x, int __y);

/* Read a character from the OS input stream.  */
extern int _kernel_osrdch (void);

/* Write a character to the OS output streams.  The return value indicates
   success or failure.  */
extern int _kernel_oswrch (int __ch);

/* Return the next byte from the file 'handle'. Return -1 on EOF.  */
extern int _kernel_osbget (unsigned __handle);

/* Write the byte 'ch' to the file 'handle'. Return success or failure.  */
extern int _kernel_osbput (int __ch, unsigned __handle);

typedef struct
{
  void * dataptr;
  int nbytes, fileptr;
  int buf_len;
  char *wild_fld;
} _kernel_osgbpb_block;

/* Read/write a number of bytes on file 'handle'. */
extern int _kernel_osgbpb (int __operation, unsigned __handle,
			   _kernel_osgbpb_block *__inout);

/* Perform an OS_Word operation.  */
extern int _kernel_osword (int __operation, int *__data);

/* Open or close a file. Open returns a file handle, close just
   indicates success/failure.  */
extern int _kernel_osfind (int __operation, char *__name);

typedef struct
{
  int load, exec;
  int start, end;
} _kernel_osfile_block;

/* Perform an OS_File operation. The _kernel_osfile_block provides
   values for registers R2-R5.  */
extern int _kernel_osfile (int __operation, const char *__name,
			   _kernel_osfile_block *__inout);

/* Perform an OS_Args operation. Generally returns the value in R2, unless
   op = 0.  */
extern int _kernel_osargs (int __operation, unsigned __handle, int __arg);

/* Call OS_CLI with the string 's'. If another application is called,
   the current application will be closed down.	  */
extern int _kernel_oscli (const char *__s);

/* Call OS_CLI with the string 's'. If chain == 0, then the current
   application is copied away and the new application executed as
   a subprogram. Control will return to the main application once the
   subprogram has completed.

   If chain == 1, then _kernel_system will not return.  */
extern int _kernel_system (const char *__string, int __chain);

/* Memory allocation functions.  */

/* Tries to allocate a block of size 'words' words. If it fails,
   it allocates the largest possible block.  */
extern unsigned _kernel_alloc (unsigned __words, void **__block);


/* Register procedures to be used by the SharedCLibrary when it
   creates to frees memory e.g. on creation of stack chunks.  */
typedef void freeproc (void *);
typedef void *allocproc (unsigned);

extern void _kernel_register_allocs (allocproc *__malloc, freeproc *__free);

/* If the heap limit != application limit, then the procedure
   registered here is used to request n bytes from the memory
   allocation routine.  */
typedef int _kernel_ExtendProc (int __n, void **__p);

_kernel_ExtendProc *_kernel_register_slotextend (_kernel_ExtendProc *__proc);

/* Language support.  */

/* Unsigned divide and remainder function. Returns the remainder in R1. */
extern unsigned
_kernel_udiv (unsigned __divisor,
	      unsigned __dividend) __attribute__ ((__const__));

/* Unsigned remainder function.  */
extern unsigned
_kernel_urem (unsigned __divisor,
	      unsigned __dividend) __attribute__ ((__const__));

/* Unsigned divide and remainder function by 10.
   Returns the remainder in R1. */
extern unsigned
_kernel_udiv10 (unsigned __dividend) __attribute__ ((__const__));

/* Signed divide and remainder function. Returns the remainder in R1. */
extern int
_kernel_sdiv (int __divisor, int __dividend) __attribute__ ((__const__));

/* Signed remainder function.  */
extern int
_kernel_srem (int __divisor, int __dividend) __attribute__ ((__const__));

/* Signed divide and remainder function by 10.
   Returns the remainder in R1. */
extern int
_kernel_sdiv10 (int __dividend) __attribute__ ((__const__));

#ifdef __cplusplus
}
#endif

#endif
