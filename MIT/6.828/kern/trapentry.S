/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
# see xv6 vectors.pl and vectors.S for reference
TRAPHANDLER_NOEC(int_divide, T_DIVIDE)		# 0
TRAPHANDLER_NOEC(int_debug, T_DEBUG)		# 1
TRAPHANDLER_NOEC(int_nmi, T_NMI)			# 2
TRAPHANDLER_NOEC(int_brkpt, T_BRKPT)		# 3
TRAPHANDLER_NOEC(int_oflow, T_OFLOW)		# 4
TRAPHANDLER_NOEC(int_bound, T_BOUND)		# 5
TRAPHANDLER_NOEC(int_illop, T_ILLOP)		# 6
TRAPHANDLER_NOEC(int_device, T_DEVICE)		# 7
TRAPHANDLER(int_dblflt, T_DBLFLT)			# 8
											# 9 is not used
TRAPHANDLER(int_tss, T_TSS)					# 10
TRAPHANDLER(int_segnp, T_SEGNP)				# 11
TRAPHANDLER(int_stack, T_STACK)				# 12
TRAPHANDLER(int_gpflt, T_GPFLT)				# 13
TRAPHANDLER(int_pgflt, T_PGFLT)				# 14
											# 15 is not used
TRAPHANDLER_NOEC(int_fperr, T_FPERR)		# 16
TRAPHANDLER(int_align, T_ALIGN)				# 17
TRAPHANDLER_NOEC(int_mchk, T_MCHK)			# 18
TRAPHANDLER_NOEC(int_simderr, T_SIMDERR)	# 19	

# hardware interrupts
TRAPHANDLER_NOEC(int_timer, IRQ_OFFSET + IRQ_TIMER)
TRAPHANDLER_NOEC(int_kbd, IRQ_OFFSET + IRQ_KBD)
TRAPHANDLER_NOEC(int_serial, IRQ_OFFSET + IRQ_SERIAL)
# TRAPHANDLER_NOEC(int_spurious, IRQ_OFFSET + IRQ_SPURIOUS)
TRAPHANDLER_NOEC(int_ide, IRQ_OFFSET + IRQ_IDE)

TRAPHANDLER_NOEC(int_syscall, T_SYSCALL)	# 48

/*
 * Lab 3: Your code here for _alltraps
 */
# see xv6 trapasm.S for reference
.global _alltraps
.type _alltraps, @function;
_alltraps:
	# push registers
	pushl %ds
	pushl %es
	# push struct Trapframe. %esp now should be the pointer to it.
	pushal
	# load DS, ES
	movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es
	# pass %esp as the parameter and call `trap()`
	pushl %esp
	call trap
	# add $4, %esp

