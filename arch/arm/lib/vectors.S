/*
 *  vectors - Generic ARM exception table code
 *
 *  Copyright (c) 1998	Dan Malek <dmalek@jlc.net>
 *  Copyright (c) 1999	Magnus Damm <kieraypc01.p.y.kie.era.ericsson.se>
 *  Copyright (c) 2000	Wolfgang Denk <wd@denx.de>
 *  Copyright (c) 2001	Alex Züpke <azu@sysgo.de>
 *  Copyright (c) 2001	Marius Gröger <mag@sysgo.de>
 *  Copyright (c) 2002	Alex Züpke <azu@sysgo.de>
 *  Copyright (c) 2002	Gary Jennejohn <garyj@denx.de>
 *  Copyright (c) 2002	Kyle Harris <kharris@nexus-tech.net>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>

/*
 *************************************************************************
 *
 * Symbol _start is referenced elsewhere, so make it global
 *
 *************************************************************************
 */

.globl _start //定义全局标号_start，供其他地方使用

/*
 *************************************************************************
 *
 * Vectors have their own section so linker script can map them easily
 *
 *************************************************************************
 */

	.section ".vectors", "ax" //定义vectors段，"ax"是 allocation  execute的缩写，表示该节区可分配并且可执行

/*
 *************************************************************************
 *
 * Exception vectors as described in ARM reference manuals
 *
 * Uses indirect branch to allow reaching handlers anywhere in memory.
 *
 *************************************************************************
 */

_start:

/*
CONFIG_SYS_DV_NOR_BOOT_CFG:	AM18xx based boards, booting in NOR Boot mode
								need a "NOR Boot Configuration Word" stored
								in the NOR Flash. This define adds this.
								More Info about this, see:
								spraba5a.pdf chapter 3.1
*/
//从NOR Flash启动的配置宏定义
#ifdef CONFIG_SYS_DV_NOR_BOOT_CFG
	.word	CONFIG_SYS_DV_NOR_BOOT_CFG
#endif
	//根据imx6ull的启动方式，由芯片内部的bootROM将uboot搬运到指定地址运行
	//这个地址由u-boot.imx头部IVT内定义的entry地址处运行。
	b	reset 						//第一个指令，跳转到reset处运行，这里使用的是b相对跳转，所以与运行位置无关
	ldr	pc, _undefined_instruction  //将_undefined_instruction处的值加载进pc指针	//中断向量表，未定义中断
	ldr	pc, _software_interrupt		//中断向量表，软中断
	ldr	pc, _prefetch_abort			//中断向量表，...
	ldr	pc, _data_abort				//中断向量表，...
	ldr	pc, _not_used				//中断向量表，...
	ldr	pc, _irq					//中断向量表，中断
	ldr	pc, _fiq					//中断向量表，快速中断

/*
 *************************************************************************
 *
 * Indirect vectors table
 *
 * Symbols referenced here must be defined somewhere else
 *
 *************************************************************************
 */

	.globl	_undefined_instruction	//定义全局标号_undefined_instruction
	.globl	_software_interrupt
	.globl	_prefetch_abort
	.globl	_data_abort
	.globl	_not_used
	.globl	_irq
	.globl	_fiq

_undefined_instruction:	.word undefined_instruction //_undefined_instruction标号处存放了 一个word(32bit)大小的变量 undefined_instruction
_software_interrupt:	.word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq
//到这里位置指令一共15条，一个指令32bit 4个字节，一共60字节(.globl伪指令并不算指令)
	.balignl 16,0xdeadbeef //以0xdeadbeef填充后续地址空间，使地址以16对齐，即地址相对于第一条指令地址偏移64字节

/*
 *************************************************************************
 *
 * Interrupt handling
 *
 *************************************************************************
 */

/* SPL interrupt handling: just hang */

//这个部分的中断处理是在使用SPL的情况下才使用的，SPL（Second Program Loader）,是由于芯片内部的SRAM空间不够存放
//整个u-boot，先将u-boot的一部分（一般是4KB左右）拷贝到芯片内部的SRAM空间,然后通过这部分代码初始化DDR之类的外接内存
//之后将u-boot拷贝到DDR运行
#ifdef CONFIG_SPL_BUILD

	.align	5 //.align  n 以2的n次方个字节对齐 即32对齐 */
undefined_instruction:
software_interrupt:
prefetch_abort:
data_abort:
not_used:
irq:
fiq:

1:
	bl	1b			/* hang and never return */

#else	/* !CONFIG_SPL_BUILD */
//不使用SPL的情况，由于imx6的bootROM可以直接初始化DDR,并将整个u-boot拷贝到DDR运行，所有并没有使用SPL
/* IRQ stack memory (calculated at run-time) + 8 bytes */
.globl IRQ_STACK_START_IN
IRQ_STACK_START_IN:
	.word	0x0badc0de //定义全局标号，标号处存储数值0x0badcode 坏的代码//预留数据


//是只有当定义了宏CONFIG_USE_IRQ的时候，才用到这两个变量，其含义也很明显，
//只有用到了中断IRQ，才会用到中断的堆栈，才有中断堆栈的起始地址。
#ifdef CONFIG_USE_IRQ
/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
	.word	0x0badc0de//预留数据

/* IRQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
	.word 0x0badc0de

#endif /* CONFIG_USE_IRQ */


@
@ 
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#define MODE_SVC 0x13
#define I_BIT	 0x80

/*
 * use bad_save_user_regs for abort/prefetch/undef/swi ...
 * use irq_save_user_regs / irq_restore_user_regs for IRQ/FIQ handling
 */
	//用来保存程序“出错”时用户态的寄存器的值。
	.macro	bad_save_user_regs  // carve out a frame on current user stack
	sub	sp, sp, #S_FRAME_SIZE   // 这个宏既然是用来“保存寄存器”的，那保存在哪呢？自然是保存在“栈”中：先令sp指针自减S_FRAME_SIZE
	stmia	sp, {r0 - r12}	    // Save user registers (now in svc mode) r0-r12, ia 表示 increase after save
								// 就是保存一个，就增加4个字节。所以，r0 - r12这13个寄存器一共13*4=52字节
								// 就被从下到上保存在刚刚划出的栈空间

	ldr	r2, IRQ_STACK_START_IN  // get values for "aborted" pc and cpsr (into parm regs), 获取终止的pc和cpsr值（即badcode），存放在r2寄存器
	ldmia	r2, {r2 - r3}		
	add	r0, sp, #S_FRAME_SIZE		@ grab pointer to old stack
	add	r5, sp, #S_SP
	mov	r1, lr
	stmia	r5, {r0 - r3}	@ save sp_SVC, lr_SVC, pc, cpsr
	mov	r0, sp		@ save current stack into r0 (param register)
	.endm

	.macro	irq_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0-r12
	@ !!!! R8 NEEDS to be saved !!!! a reserved stack spot would be good.
	add	r8, sp, #S_PC
	stmdb	r8, {sp, lr}^		@ Calling SP, LR
	str	lr, [r8, #0]		@ Save calling PC
	mrs	r6, spsr
	str	r6, [r8, #4]		@ Save CPSR
	str	r0, [r8, #8]		@ Save OLD_R0
	mov	r0, sp
	.endm

	.macro	irq_restore_user_regs
	ldmia	sp, {r0 - lr}^			@ Calling r0 - lr
	mov	r0, r0
	ldr	lr, [sp, #S_PC]			@ Get PC
	add	sp, sp, #S_FRAME_SIZE
	subs	pc, lr, #4		@ return & move spsr_svc into cpsr
	.endm

	.macro get_bad_stack
	ldr	r13, IRQ_STACK_START_IN		@ setup our mode stack

	str	lr, [r13]	@ save caller lr in position 0 of saved stack
	mrs	lr, spsr	@ get the spsr
	str	lr, [r13, #4]	@ save spsr in position 1 of saved stack
	mov	r13, #MODE_SVC	@ prepare SVC-Mode
	@ msr	spsr_c, r13
	msr	spsr, r13	@ switch modes, make sure moves will execute
	mov	lr, pc		@ capture return pc
	movs	pc, lr		@ jump to next instruction & switch modes.
	.endm

	.macro get_irq_stack			@ setup IRQ stack
	ldr	sp, IRQ_STACK_START
	.endm

	.macro get_fiq_stack			@ setup FIQ stack
	ldr	sp, FIQ_STACK_START
	.endm

/*
 * exception handlers
 */

	.align  5
undefined_instruction:
	get_bad_stack
	bad_save_user_regs
	bl	do_undefined_instruction

	.align	5
software_interrupt:
	get_bad_stack
	bad_save_user_regs
	bl	do_software_interrupt

	.align	5
prefetch_abort:
	get_bad_stack
	bad_save_user_regs
	bl	do_prefetch_abort

	.align	5
data_abort:
	get_bad_stack
	bad_save_user_regs
	bl	do_data_abort

	.align	5
not_used:
	get_bad_stack
	bad_save_user_regs
	bl	do_not_used

#ifdef CONFIG_USE_IRQ

	.align	5
irq:
	get_irq_stack
	irq_save_user_regs
	bl	do_irq
	irq_restore_user_regs

	.align	5
fiq:
	get_fiq_stack
	/* someone ought to write a more effiction fiq_save_user_regs */
	irq_save_user_regs
	bl	do_fiq
	irq_restore_user_regs

#else

	.align	5
irq:
	get_bad_stack
	bad_save_user_regs
	bl	do_irq

	.align	5
fiq:
	get_bad_stack
	bad_save_user_regs
	bl	do_fiq

#endif /* CONFIG_USE_IRQ */

#endif	/* CONFIG_SPL_BUILD */
