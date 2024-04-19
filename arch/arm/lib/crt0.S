/*
 *  crt0 - C-runtime startup Code for ARM U-Boot
 */

#include <config.h>
#include <asm-offsets.h>
#include <linux/linkage.h>
#ifdef CONFIG_CPU_V7M
#include <asm/armv7m.h>
#endif

/*
 * 此文件处理 U-Boot 启动的与目标无关的阶段，其中需要 C 运行时环境。
 * 其入口点为 _main，并从目标的 start.S 文件分支到此处。
 *
 * _main 执行顺序如下：
 *
 * 1. 设置用于调用 board_init_f() 的初始环境。
 *    此环境仅提供堆栈和存储 GD（'global data'）结构的地方，
 *    它们都位于一些可用的 RAM（SRAM、锁定缓存等）中。
 *    在此上下文中，可变的全局数据（初始化或未初始化的 BSS）不可用；
 *    仅可用常量初始化数据。在调用 board_init_f() 之前，GD 应清零。
 *
 * 2. 调用 board_init_f()。此函数准备硬件以执行从系统 RAM（DRAM、DDR...）中执行。
 *    由于系统 RAM 可能尚不可用，board_init_f() 必须使用当前的 GD 来存储必须传递到后续阶段的任何数据。
 *    这些数据包括重定位目的地、未来的堆栈和未来的 GD 位置。
 *
 * 3. 设置中间环境，其中堆栈和 GD 是由 board_init_f() 在系统 RAM 中分配的，
 *    但是 BSS 和初始化的非常量数据仍然不可用。
 *
 * 4a. 对于 U-Boot 本身（不是 SPL），调用 relocate_code()。
 *     此函数将 U-Boot 从当前位置重新定位到由 board_init_f() 计算的重定位目的地。
 *
 * 4b. 对于 SPL，board_init_f() 仅返回（到 crt0）。在 SPL 中没有代码重定位。
 *
 * 5. 为调用 board_init_r() 设置最终环境。
 *    此环境具有 BSS（初始化为 0）、初始化的非常量数据（初始化为它们的预期值），
 *    以及系统 RAM 中的堆栈（对于将堆栈和 GD 移入 RAM 的 SPL 是可选的 - 请参见 CONFIG_SPL_STACK_R）。
 *    GD 保留了由 board_init_f() 设置的值。
 *
 * 6. 对于 U-Boot 本身（不是 SPL），在此时一些 CPU 可能还有一些与内存有关的工作要做，因此调用 c_runtime_cpu_setup。
 *
 * 7. 分支到 board_init_r()。
 *
 * 有关更多信息，请参阅 README 中的 'Board Initialisation Flow'。
 */

/*
 * entry point of crt0 sequence
 */

ENTRY(_main)


/*
 *设置初始 C 运行时环境并调用 board_init_f(0) 函数。
 */

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_STACK)
	ldr	sp, =(CONFIG_SPL_STACK)
#else
	ldr	sp, =(CONFIG_SYS_INIT_SP_ADDR)
#endif
#if defined(CONFIG_CPU_V7M)	/* v7M 禁止使用 SP 作为 BIC 的目的地 */
	mov	r3, sp
	bic	r3, r3, #7
	mov	sp, r3
#else
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
#endif
	mov	r0, sp
	bl	board_init_f_alloc_reserve
	mov	sp, r0
	/* set up gd here, outside any C code */
	mov	r9, r0
	bl	board_init_f_init_reserve

	mov	r0, #0
	bl	board_init_f

#if ! defined(CONFIG_SPL_BUILD)

/*
 * 设置中间环境（新的堆栈指针和全局数据结构），然后调用 relocate_code(addr_moni) 函数。
 * 这里的技巧是，我们将返回 'here'，但是已经重定位过了。
 */

	ldr	sp, [r9, #GD_START_ADDR_SP]	/* sp = gd->start_addr_sp */
#if defined(CONFIG_CPU_V7M)	/* v7M forbids using SP as BIC destination */
	mov	r3, sp
	bic	r3, r3, #7
	mov	sp, r3
#else
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
#endif
	ldr	r9, [r9, #GD_BD]		/* r9 = gd->bd */
	sub	r9, r9, #GD_SIZE		/* new GD is below bd */

	adr	lr, here				/*将当前地址（here）加载到链接寄存器（lr）中*/
	ldr	r0, [r9, #GD_RELOC_OFF]		/* r0 = gd->reloc_off */
	add	lr, lr, r0				/*将lr中的地址偏移，这样的话返回的时候就是重定位之后的地址了*/
/*
 * lr寄存器的值加上 r0寄存器的值，重新赋值给 lr寄存器。因为接下来要重定位
 * 代码，也就是把代码拷贝到新的地方去 (现在的 uboot存放的起始地址为 0X87800000，下面要
 * 将 uboot拷贝到 DDR最后面的地址空间出，将 0X87800000开始的内存空出来 )，其中就包括
 * here，因此 lr中的 here要使用重定位后的位置。
 */

#if defined(CONFIG_CPU_V7M)
	orr	lr, #1				/* As required by Thumb-only */
#endif
	ldr	r0, [r9, #GD_RELOCADDR]		/* r0 = gd->relocaddr，gd->relocaddr为重定位后uboot的首地址 */
	b	relocate_code
here:
/*
 * now relocate vectors
 */

	bl	relocate_vectors

/* Set up final (full) environment */

	bl	c_runtime_cpu_setup	/* we still call old routine here */
#endif
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_FRAMEWORK)
# ifdef CONFIG_SPL_BUILD
	/* Use a DRAM stack for the rest of SPL, if requested */
	bl	spl_relocate_stack_gd
	cmp	r0, #0
	movne	sp, r0
	movne	r9, r0
# endif
	ldr	r0, =__bss_start	/* this is auto-relocated! */

#ifdef CONFIG_USE_ARCH_MEMSET
	ldr	r3, =__bss_end		/* this is auto-relocated! */
	mov	r1, #0x00000000		/* prepare zero to clear BSS */

	subs	r2, r3, r0		/* r2 = memset len */
	bl	memset
#else
	ldr	r1, =__bss_end		/* this is auto-relocated! */
	mov	r2, #0x00000000		/* prepare zero to clear BSS */

clbss_l:cmp	r0, r1			/* while not at end of BSS */
#if defined(CONFIG_CPU_V7M)
	itt	lo
#endif
	strlo	r2, [r0]		/* clear 32-bit BSS word */
	addlo	r0, r0, #4		/* move to next */
	blo	clbss_l
#endif

#if ! defined(CONFIG_SPL_BUILD)
	bl coloured_LED_init
	bl red_led_on
#endif
	/* call board_init_r(gd_t *id, ulong dest_addr) */
	mov     r0, r9                  /* gd_t */
	ldr	r1, [r9, #GD_RELOCADDR]	/* dest_addr */
	/* call board_init_r */
#if defined(CONFIG_SYS_THUMB_BUILD)
	ldr	lr, =board_init_r	/* this is auto-relocated! */
	bx	lr
#else
	ldr	pc, =board_init_r	/* this is auto-relocated! */
#endif
	/* we should not return here. */
#endif

ENDPROC(_main)
