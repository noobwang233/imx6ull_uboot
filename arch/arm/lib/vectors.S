#include <config.h>

.globl _start //定义全局标号_start，供其他地方使用


/*
 *************************************************************************
 * 中断向量表
 *************************************************************************
 */
	/* 为中断向量表单独定义一个段，方便链接脚本的对其map */
	.section ".vectors", "ax" //定义vectors段，"ax"是 allocation  execute的缩写，表示该节区可分配并且可执行

_start: //程序入口
	//根据imx6ull的启动方式，由芯片内部的bootROM将uboot搬运到指定地址运行
	//这个地址由u-boot.imx头部IVT内定义的entry地址处运行。
	b	reset 						//第一个指令，跳转到reset处运行，这里使用的是b相对跳转，所以与运行位置无关
	/* 使用间接，使pc跳转到对应的中断向量处，可以让此代码在内存任何位置都可以运行 */
	ldr	pc, _undefined_instruction  //使用间接跳转，跳转到_undefined_instruction处	//中断向量表，未定义中断
	ldr	pc, _software_interrupt		//中断向量表，软中断
	ldr	pc, _prefetch_abort			//中断向量表，...
	ldr	pc, _data_abort				//中断向量表，...
	ldr	pc, _not_used				//中断向量表，...
	ldr	pc, _irq					//中断向量表，中断
	ldr	pc, _fiq					//中断向量表，快速中断

	/* 间接的中断向量表，这里所列出的的标号需要被定义 */
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
 * 中断处理函数
 *************************************************************************
 */

/*
 * 这个部分的中断处理是在使用SPL的情况下才使用的，SPL（Second Program Loader）,是由于芯片内部的SRAM空间不够存放
 * 整个u-boot，先将u-boot的一部分（一般是4KB左右）拷贝到芯片内部的SRAM空间,然后通过这部分代码初始化DDR之类的外接内存
 * 之后将u-boot拷贝到DDR运行
 */
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
	bl	1b			/* 在此情况下的所有中断处理函数就是死循环 */

#else	/* 不使用SPL的情况 由于imx6的bootROM可以直接初始化DDR,并将整个u-boot拷贝到DDR运行，所有并没有使用SPL */

/* IRQ stack memory (calculated at run-time) + 8 bytes */
.globl IRQ_STACK_START_IN
IRQ_STACK_START_IN:
	.word	0x0badc0de //定义全局标号，为irq的栈顶指针（预留数据为0x0badc0de）

/*
 * CONFIG_USE_IRQ 配置是否使用IRQ, 只有用到了中断IRQ，才会用到中断的堆栈，才有中断堆栈的起始地址。
 */
#ifdef CONFIG_USE_IRQ
/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
	.word	0x0badc0de //定义全局标号，为irq的栈起始地址（预留数据为0x0badc0de）

/* FIQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
	.word 0x0badc0de //定义全局标号，为fiq的栈起始地址（预留数据为0x0badc0de）

#endif /* CONFIG_USE_IRQ */

/* 寄存器现场，大小72字节 */
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
 * 使用 bad_save_user_regs 来保存 abort/prefetch/undef/swi ... 下的寄存器现场
 * 使用 irq_save_user_regs 和 irq_restore_user_regs 来保存和恢复进入IRQ和FIQ的寄存器现场 
 */
	.macro	bad_save_user_regs  // 定义一个宏指令，用来保存程序“出错”时用户态的寄存器的值。此时sp指针为对应模式的栈指针
	sub	sp, sp, #S_FRAME_SIZE   // 这个宏既然是用来“保存寄存器”的，那保存在哪呢？自然是保存在“栈”中：先令sp指针自减S_FRAME_SIZE
	stmia	sp, {r0 - r12}	    // 保存用户寄存器 (现在处于svc模式) r0-r12, r0-r12是用户模式和其他模式（除了fiq模式）共用的
                                // ia 表示 increase after save 就是保存一个，就增加4个字节。
                                // 所以，r0 - r12这13个寄存器一共13*4=52字节
    // 获取终止时的 pc and cpsr (放入参数寄存器)
	ldr	r2, IRQ_STACK_START_IN  //TODO
	ldmia	r2, {r2 - r3}		// 将pc 和 cpsr 出栈到 r2 - r3 （作为第三和第四个参数）
	add	r0, sp, #S_FRAME_SIZE	// 将r0赋值为调用bad_save_user_regs之前的sp
	add	r5, sp, #S_SP           // 将r5赋值为 sp + S_SP, 将sp增加到sp处，准备保存之后的寄存器
	mov	r1, lr                  // 将lr保存到r1,作为函数第二个参数
	stmia	r5, {r0 - r3}	    // 保存 sp_SVC, lr_SVC, pc, cpsr
	mov	r0, sp		            // 将此时的sp保存到r0,作为函数第一个参数
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
