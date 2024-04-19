/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/compiler.h>
#include <version.h>
#include <console.h>
#include <environment.h>
#include <dm.h>
#include <fdtdec.h>
#include <fs.h>
#if defined(CONFIG_CMD_IDE)
#include <ide.h>
#endif
#include <i2c.h>
#include <initcall.h>
#include <logbuff.h>
#include <malloc.h>
#include <mapmem.h>

/* TODO: Can we move these into arch/ headers? */
#ifdef CONFIG_8xx
#include <mpc8xx.h>
#endif
#ifdef CONFIG_5xx
#include <mpc5xx.h>
#endif
#ifdef CONFIG_MPC5xxx
#include <mpc5xxx.h>
#endif
#if defined(CONFIG_MP) && (defined(CONFIG_MPC86xx) || defined(CONFIG_E500))
#include <asm/mp.h>
#endif

#include <os.h>
#include <post.h>
#include <spi.h>
#include <status_led.h>
#include <timer.h>
#include <trace.h>
#include <video.h>
#include <watchdog.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/sections.h>
#if defined(CONFIG_X86) || defined(CONFIG_ARC)
#include <asm/init_helpers.h>
#include <asm/relocate.h>
#endif
#ifdef CONFIG_SANDBOX
#include <asm/state.h>
#endif
#include <dm/root.h>
#include <linux/compiler.h>

/*
 * Pointer to initial global data area
 *
 * Here we initialize it if needed.
 */
#ifdef XTRN_DECLARE_GLOBAL_DATA_PTR
#undef	XTRN_DECLARE_GLOBAL_DATA_PTR
#define XTRN_DECLARE_GLOBAL_DATA_PTR	/* empty = allocate here */
DECLARE_GLOBAL_DATA_PTR = (gd_t *) (CONFIG_SYS_INIT_GD_ADDR);
#else
DECLARE_GLOBAL_DATA_PTR;
#endif

/*
 * TODO(sjg@chromium.org): IMO this code should be
 * refactored to a single function, something like:
 *
 * void led_set_state(enum led_colour_t colour, int on);
 */
/************************************************************************
 * Coloured LED functionality
 ************************************************************************
 * May be supplied by boards if desired
 */
__weak void coloured_LED_init(void) {}
__weak void red_led_on(void) {}
__weak void red_led_off(void) {}
__weak void green_led_on(void) {}
__weak void green_led_off(void) {}
__weak void yellow_led_on(void) {}
__weak void yellow_led_off(void) {}
__weak void blue_led_on(void) {}
__weak void blue_led_off(void) {}

/*
 * Why is gd allocated a register? Prior to reloc it might be better to
 * just pass it around to each function in this file?
 *
 * After reloc one could argue that it is hardly used and doesn't need
 * to be in a register. Or if it is it should perhaps hold pointers to all
 * global data for all modules, so that post-reloc we can avoid the massive
 * literal pool we get on ARM. Or perhaps just encourage each module to use
 * a structure...
 */

/*
 * Could the CONFIG_SPL_BUILD infection become a flag in gd?
 */

#if defined(CONFIG_WATCHDOG) || defined(CONFIG_HW_WATCHDOG)
static int init_func_watchdog_init(void)
{
# if defined(CONFIG_HW_WATCHDOG) && (defined(CONFIG_BLACKFIN) || \
	defined(CONFIG_M68K) || defined(CONFIG_MICROBLAZE) || \
	defined(CONFIG_SH) || defined(CONFIG_AT91SAM9_WATCHDOG) || \
	defined(CONFIG_IMX_WATCHDOG))
	hw_watchdog_init();
# endif
	puts("       Watchdog enabled\n");
	WATCHDOG_RESET();

	return 0;
}

int init_func_watchdog_reset(void)
{
	WATCHDOG_RESET();

	return 0;
}
#endif /* CONFIG_WATCHDOG */

__weak void board_add_ram_info(int use_default)
{
	/* please define platform specific board_add_ram_info() */
}

static int init_baud_rate(void)
{
	gd->baudrate = getenv_ulong("baudrate", 10, CONFIG_BAUDRATE);
	return 0;
}

static int display_text_info(void)
{
#if !defined(CONFIG_SANDBOX) && !defined(CONFIG_EFI_APP)
	ulong bss_start, bss_end, text_base;

	bss_start = (ulong)&__bss_start;
	bss_end = (ulong)&__bss_end;

#ifdef CONFIG_SYS_TEXT_BASE
	text_base = CONFIG_SYS_TEXT_BASE;
#else
	text_base = CONFIG_SYS_MONITOR_BASE;
#endif

	debug("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
		text_base, bss_start, bss_end);
#endif

#ifdef CONFIG_USE_IRQ
	debug("IRQ Stack: %08lx\n", IRQ_STACK_START);
	debug("FIQ Stack: %08lx\n", FIQ_STACK_START);
#endif

	return 0;
}

static int announce_dram_init(void)
{
	puts("DRAM:  ");
	return 0;
}

#if defined(CONFIG_MIPS) || defined(CONFIG_PPC) || defined(CONFIG_M68K)
static int init_func_ram(void)
{
#ifdef	CONFIG_BOARD_TYPES
	int board_type = gd->board_type;
#else
	int board_type = 0;	/* use dummy arg */
#endif

	gd->ram_size = initdram(board_type);

	if (gd->ram_size > 0)
		return 0;

	puts("*** failed ***\n");
	return 1;
}
#endif

static int show_dram_config(void)
{
	unsigned long long size;

#ifdef CONFIG_NR_DRAM_BANKS
	int i;

	debug("\nRAM Configuration:\n");
	for (i = size = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		size += gd->bd->bi_dram[i].size;
		debug("Bank #%d: %llx ", i,
		      (unsigned long long)(gd->bd->bi_dram[i].start));
#ifdef DEBUG
		print_size(gd->bd->bi_dram[i].size, "\n");
#endif
	}
	debug("\nDRAM:  ");
#else
	size = gd->ram_size;
#endif

	print_size(size, "");
	board_add_ram_info(0);
	putc('\n');

	return 0;
}

__weak void dram_init_banksize(void)
{
#if defined(CONFIG_NR_DRAM_BANKS) && defined(CONFIG_SYS_SDRAM_BASE)
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = get_effective_memsize();
#endif
}

#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SYS_I2C)
static int init_func_i2c(void)
{
	puts("I2C:   ");
#ifdef CONFIG_SYS_I2C
	i2c_init_all();
#else
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#endif
	puts("ready\n");
	return 0;
}
#endif

#if defined(CONFIG_HARD_SPI)
static int init_func_spi(void)
{
	puts("SPI:   ");
	spi_init();
	puts("ready\n");
	return 0;
}
#endif

__maybe_unused
static int zero_global_data(void)
{
	memset((void *)gd, '\0', sizeof(gd_t));

	return 0;
}

static int setup_mon_len(void)
{
#if defined(__ARM__) || defined(__MICROBLAZE__)
	gd->mon_len = (ulong)&__bss_end - (ulong)_start;
#elif defined(CONFIG_SANDBOX) || defined(CONFIG_EFI_APP)
	gd->mon_len = (ulong)&_end - (ulong)_init;
#elif defined(CONFIG_BLACKFIN) || defined(CONFIG_NIOS2)
	gd->mon_len = CONFIG_SYS_MONITOR_LEN;
#elif defined(CONFIG_NDS32)
	gd->mon_len = (ulong)(&__bss_end) - (ulong)(&_start);
#else
	/* TODO: use (ulong)&__bss_end - (ulong)&__text_start; ? */
	gd->mon_len = (ulong)&__bss_end - CONFIG_SYS_MONITOR_BASE;
#endif
	return 0;
}

__weak int arch_cpu_init(void)
{
	return 0;
}

#ifdef CONFIG_SANDBOX
static int setup_ram_buf(void)
{
	struct sandbox_state *state = state_get_current();

	gd->arch.ram_buf = state->ram_buf;
	gd->ram_size = state->ram_size;

	return 0;
}
#endif

/* Get the top of usable RAM */
__weak ulong board_get_usable_ram_top(ulong total_size)
{
#ifdef CONFIG_SYS_SDRAM_BASE
	/*
	 * Detect whether we have so much RAM that it goes past the end of our
	 * 32-bit address space. If so, clip the usable RAM so it doesn't.
	 */
	if (gd->ram_top < CONFIG_SYS_SDRAM_BASE)
		/*
		 * Will wrap back to top of 32-bit space when reservations
		 * are made.
		 */
		return 0;
#endif
	return gd->ram_top;
}

__weak phys_size_t board_reserve_ram_top(phys_size_t ram_size)
{
#ifdef CONFIG_SYS_MEM_TOP_HIDE
	return ram_size - CONFIG_SYS_MEM_TOP_HIDE;
#else
	return ram_size;
#endif
}

static int setup_dest_addr(void)
{
	debug("Monitor len: %08lX\n", gd->mon_len);
	/*
	 * Ram is setup, size stored in gd !!
	 */
	debug("Ram size: %08lX\n", (ulong)gd->ram_size);
#ifdef CONFIG_SYS_MEM_RESERVE_SECURE
	/* Reserve memory for secure MMU tables, and/or security monitor */
	gd->ram_size -= CONFIG_SYS_MEM_RESERVE_SECURE;
	/*
	 * Record secure memory location. Need recalcuate if memory splits
	 * into banks, or the ram base is not zero.
	 */
	gd->secure_ram = gd->ram_size;
#endif
	/*
	 * Subtract specified amount of memory to hide so that it won't
	 * get "touched" at all by U-Boot. By fixing up gd->ram_size
	 * the Linux kernel should now get passed the now "corrected"
	 * memory size and won't touch it either. This has been used
	 * by arch/powerpc exclusively. Now ARMv8 takes advantage of
	 * thie mechanism. If memory is split into banks, addresses
	 * need to be calculated.
	 */
	gd->ram_size = board_reserve_ram_top(gd->ram_size);

#ifdef CONFIG_SYS_SDRAM_BASE
	gd->ram_top = CONFIG_SYS_SDRAM_BASE;
#endif
	gd->ram_top += get_effective_memsize();
	gd->ram_top = board_get_usable_ram_top(gd->mon_len);
	gd->relocaddr = gd->ram_top;
	debug("Ram top: %08lX\n", (ulong)gd->ram_top);
#if defined(CONFIG_MP) && (defined(CONFIG_MPC86xx) || defined(CONFIG_E500))
	/*
	 * We need to make sure the location we intend to put secondary core
	 * boot code is reserved and not used by any part of u-boot
	 */
	if (gd->relocaddr > determine_mp_bootpg(NULL)) {
		gd->relocaddr = determine_mp_bootpg(NULL);
		debug("Reserving MP boot page to %08lx\n", gd->relocaddr);
	}
#endif
	return 0;
}

#if defined(CONFIG_SPARC)
static int reserve_prom(void)
{
	/* defined in arch/sparc/cpu/leon?/prom.c */
	extern void *__prom_start_reloc;
	int size = 8192; /* page table = 2k, prom = 6k */
	gd->relocaddr -= size;
	__prom_start_reloc = map_sysmem(gd->relocaddr + 2048, size - 2048);
	debug("Reserving %dk for PROM and page table at %08lx\n", size,
		gd->relocaddr);
	return 0;
}
#endif

#if defined(CONFIG_LOGBUFFER) && !defined(CONFIG_ALT_LB_ADDR)
static int reserve_logbuffer(void)
{
	/* reserve kernel log buffer */
	gd->relocaddr -= LOGBUFF_RESERVE;
	debug("Reserving %dk for kernel logbuffer at %08lx\n", LOGBUFF_LEN,
		gd->relocaddr);
	return 0;
}
#endif

#ifdef CONFIG_PRAM
/* reserve protected RAM */
static int reserve_pram(void)
{
	ulong reg;

	reg = getenv_ulong("pram", 10, CONFIG_PRAM);
	gd->relocaddr -= (reg << 10);		/* size is in kB */
	debug("Reserving %ldk for protected RAM at %08lx\n", reg,
	      gd->relocaddr);
	return 0;
}
#endif /* CONFIG_PRAM */

/* Round memory pointer down to next 4 kB limit */
static int reserve_round_4k(void)
{
	gd->relocaddr &= ~(4096 - 1);
	return 0;
}

#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF)) && \
		defined(CONFIG_ARM)
static int reserve_mmu(void)
{
	/* reserve TLB table */
	gd->arch.tlb_size = PGTABLE_SIZE;
	gd->relocaddr -= gd->arch.tlb_size;

	/* round down to next 64 kB limit */
	gd->relocaddr &= ~(0x10000 - 1);

	gd->arch.tlb_addr = gd->relocaddr;
	debug("TLB table from %08lx to %08lx\n", gd->arch.tlb_addr,
	      gd->arch.tlb_addr + gd->arch.tlb_size);
	return 0;
}
#endif

#ifdef CONFIG_DM_VIDEO
static int reserve_video(void)
{
	ulong addr;
	int ret;

	addr = gd->relocaddr;
	ret = video_reserve(&addr);
	if (ret)
		return ret;
	gd->relocaddr = addr;

	return 0;
}
#else

# ifdef CONFIG_LCD
static int reserve_lcd(void)
{
#  ifdef CONFIG_FB_ADDR
	gd->fb_base = CONFIG_FB_ADDR;
#  else
	/* reserve memory for LCD display (always full pages) */
	gd->relocaddr = lcd_setmem(gd->relocaddr);
	gd->fb_base = gd->relocaddr;
#  endif /* CONFIG_FB_ADDR */

	return 0;
}
# endif /* CONFIG_LCD */

# if defined(CONFIG_VIDEO) && (!defined(CONFIG_PPC) || defined(CONFIG_8xx)) && \
		!defined(CONFIG_ARM) && !defined(CONFIG_X86) && \
		!defined(CONFIG_BLACKFIN) && !defined(CONFIG_M68K)
static int reserve_legacy_video(void)
{
	/* reserve memory for video display (always full pages) */
	gd->relocaddr = video_setmem(gd->relocaddr);
	gd->fb_base = gd->relocaddr;

	return 0;
}
# endif
#endif /* !CONFIG_DM_VIDEO */

static int reserve_trace(void)
{
#ifdef CONFIG_TRACE
	gd->relocaddr -= CONFIG_TRACE_BUFFER_SIZE;
	gd->trace_buff = map_sysmem(gd->relocaddr, CONFIG_TRACE_BUFFER_SIZE);
	debug("Reserving %dk for trace data at: %08lx\n",
	      CONFIG_TRACE_BUFFER_SIZE >> 10, gd->relocaddr);
#endif

	return 0;
}

static int reserve_uboot(void)
{
	/*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
	gd->relocaddr -= gd->mon_len;
	gd->relocaddr &= ~(4096 - 1);
#ifdef CONFIG_E500
	/* round down to next 64 kB limit so that IVPR stays aligned */
	gd->relocaddr &= ~(65536 - 1);
#endif

	debug("Reserving %ldk for U-Boot at: %08lx\n", gd->mon_len >> 10,
	      gd->relocaddr);

	gd->start_addr_sp = gd->relocaddr;

	return 0;
}

#ifndef CONFIG_SPL_BUILD
/* reserve memory for malloc() area */
static int reserve_malloc(void)
{
	gd->start_addr_sp = gd->start_addr_sp - TOTAL_MALLOC_LEN;
	debug("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, gd->start_addr_sp);
	return 0;
}

/* (permanently) allocate a Board Info struct */
static int reserve_board(void)
{
	if (!gd->bd) {
		gd->start_addr_sp -= sizeof(bd_t);
		gd->bd = (bd_t *)map_sysmem(gd->start_addr_sp, sizeof(bd_t));
		memset(gd->bd, '\0', sizeof(bd_t));
		debug("Reserving %zu Bytes for Board Info at: %08lx\n",
		      sizeof(bd_t), gd->start_addr_sp);
	}
	return 0;
}
#endif

static int setup_machine(void)
{
#ifdef CONFIG_MACH_TYPE
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE; /* board id for Linux */
#endif
	return 0;
}

static int reserve_global_data(void)
{
	gd->start_addr_sp -= sizeof(gd_t);
	gd->new_gd = (gd_t *)map_sysmem(gd->start_addr_sp, sizeof(gd_t));
	debug("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof(gd_t), gd->start_addr_sp);
	return 0;
}

static int reserve_fdt(void)
{
#ifndef CONFIG_OF_EMBED
	/*
	 * If the device tree is sitting immediately above our image then we
	 * must relocate it. If it is embedded in the data section, then it
	 * will be relocated with other data.
	 */
	if (gd->fdt_blob) {
		gd->fdt_size = ALIGN(fdt_totalsize(gd->fdt_blob) + 0x1000, 32);

		gd->start_addr_sp -= gd->fdt_size;
		gd->new_fdt = map_sysmem(gd->start_addr_sp, gd->fdt_size);
		debug("Reserving %lu Bytes for FDT at: %08lx\n",
		      gd->fdt_size, gd->start_addr_sp);
	}
#endif

	return 0;
}

int arch_reserve_stacks(void)
{
	return 0;
}

static int reserve_stacks(void)
{
	/* make stack pointer 16-byte aligned */
	gd->start_addr_sp -= 16;
	gd->start_addr_sp &= ~0xf;

	/*
	 * let the architecture-specific code tailor gd->start_addr_sp and
	 * gd->irq_sp
	 */
	return arch_reserve_stacks();
}

static int display_new_sp(void)
{
	debug("New Stack Pointer is: %08lx\n", gd->start_addr_sp);

	return 0;
}

#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_MIPS)
static int setup_board_part1(void)
{
	bd_t *bd = gd->bd;

	/*
	 * Save local variables to board info struct
	 */
	bd->bi_memstart = CONFIG_SYS_SDRAM_BASE;	/* start of memory */
	bd->bi_memsize = gd->ram_size;			/* size in bytes */

#ifdef CONFIG_SYS_SRAM_BASE
	bd->bi_sramstart = CONFIG_SYS_SRAM_BASE;	/* start of SRAM */
	bd->bi_sramsize = CONFIG_SYS_SRAM_SIZE;		/* size  of SRAM */
#endif

#if defined(CONFIG_8xx) || defined(CONFIG_MPC8260) || defined(CONFIG_5xx) || \
		defined(CONFIG_E500) || defined(CONFIG_MPC86xx)
	bd->bi_immr_base = CONFIG_SYS_IMMR;	/* base  of IMMR register     */
#endif
#if defined(CONFIG_MPC5xxx) || defined(CONFIG_M68K)
	bd->bi_mbar_base = CONFIG_SYS_MBAR;	/* base of internal registers */
#endif
#if defined(CONFIG_MPC83xx)
	bd->bi_immrbar = CONFIG_SYS_IMMR;
#endif

	return 0;
}
#endif

#if defined(CONFIG_PPC) || defined(CONFIG_M68K)
static int setup_board_part2(void)
{
	bd_t *bd = gd->bd;

	bd->bi_intfreq = gd->cpu_clk;	/* Internal Freq, in Hz */
	bd->bi_busfreq = gd->bus_clk;	/* Bus Freq,      in Hz */
#if defined(CONFIG_CPM2)
	bd->bi_cpmfreq = gd->arch.cpm_clk;
	bd->bi_brgfreq = gd->arch.brg_clk;
	bd->bi_sccfreq = gd->arch.scc_clk;
	bd->bi_vco = gd->arch.vco_out;
#endif /* CONFIG_CPM2 */
#if defined(CONFIG_MPC512X)
	bd->bi_ipsfreq = gd->arch.ips_clk;
#endif /* CONFIG_MPC512X */
#if defined(CONFIG_MPC5xxx)
	bd->bi_ipbfreq = gd->arch.ipb_clk;
	bd->bi_pcifreq = gd->pci_clk;
#endif /* CONFIG_MPC5xxx */
#if defined(CONFIG_M68K) && defined(CONFIG_PCI)
	bd->bi_pcifreq = gd->pci_clk;
#endif
#if defined(CONFIG_EXTRA_CLOCK)
	bd->bi_inpfreq = gd->arch.inp_clk;	/* input Freq in Hz */
	bd->bi_vcofreq = gd->arch.vco_clk;	/* vco Freq in Hz */
	bd->bi_flbfreq = gd->arch.flb_clk;	/* flexbus Freq in Hz */
#endif

	return 0;
}
#endif

#ifdef CONFIG_SYS_EXTBDINFO
static int setup_board_extra(void)
{
	bd_t *bd = gd->bd;

	strncpy((char *) bd->bi_s_version, "1.2", sizeof(bd->bi_s_version));
	strncpy((char *) bd->bi_r_version, U_BOOT_VERSION,
		sizeof(bd->bi_r_version));

	bd->bi_procfreq = gd->cpu_clk;	/* Processor Speed, In Hz */
	bd->bi_plb_busfreq = gd->bus_clk;
#if defined(CONFIG_405GP) || defined(CONFIG_405EP) || \
		defined(CONFIG_440EP) || defined(CONFIG_440GR) || \
		defined(CONFIG_440EPX) || defined(CONFIG_440GRX)
	bd->bi_pci_busfreq = get_PCI_freq();
	bd->bi_opbfreq = get_OPB_freq();
#elif defined(CONFIG_XILINX_405)
	bd->bi_pci_busfreq = get_PCI_freq();
#endif

	return 0;
}
#endif

#ifdef CONFIG_POST
static int init_post(void)
{
	post_bootmode_init();
	post_run(NULL, POST_ROM | post_bootmode_get(0));

	return 0;
}
#endif

static int setup_dram_config(void)
{
	/* Ram is board specific, so move it to board code ... */
	dram_init_banksize();

	return 0;
}

static int reloc_fdt(void)
{
#ifndef CONFIG_OF_EMBED
	if (gd->flags & GD_FLG_SKIP_RELOC)
		return 0;
	if (gd->new_fdt) {
		memcpy(gd->new_fdt, gd->fdt_blob, gd->fdt_size);
		gd->fdt_blob = gd->new_fdt;
	}
#endif

	return 0;
}

static int setup_reloc(void)
{
	if (gd->flags & GD_FLG_SKIP_RELOC) {
		debug("Skipping relocation due to flag\n");
		return 0;
	}

#ifdef CONFIG_SYS_TEXT_BASE
	gd->reloc_off = gd->relocaddr - CONFIG_SYS_TEXT_BASE;
#ifdef CONFIG_M68K
	/*
	 * On all ColdFire arch cpu, monitor code starts always
	 * just after the default vector table location, so at 0x400
	 */
	gd->reloc_off = gd->relocaddr - (CONFIG_SYS_TEXT_BASE + 0x400);
#endif
#endif
	memcpy(gd->new_gd, (char *)gd, sizeof(gd_t));

	debug("Relocation Offset is: %08lx\n", gd->reloc_off);
	debug("Relocating to %08lx, new gd at %08lx, sp at %08lx\n",
	      gd->relocaddr, (ulong)map_to_sysmem(gd->new_gd),
	      gd->start_addr_sp);

	return 0;
}

/* ARM calls relocate_code from its crt0.S */
#if !defined(CONFIG_ARM) && !defined(CONFIG_SANDBOX)

static int jump_to_copy(void)
{
	if (gd->flags & GD_FLG_SKIP_RELOC)
		return 0;
	/*
	 * x86 is special, but in a nice way. It uses a trampoline which
	 * enables the dcache if possible.
	 *
	 * For now, other archs use relocate_code(), which is implemented
	 * similarly for all archs. When we do generic relocation, hopefully
	 * we can make all archs enable the dcache prior to relocation.
	 */
#if defined(CONFIG_X86) || defined(CONFIG_ARC)
	/*
	 * SDRAM and console are now initialised. The final stack can now
	 * be setup in SDRAM. Code execution will continue in Flash, but
	 * with the stack in SDRAM and Global Data in temporary memory
	 * (CPU cache)
	 */
	arch_setup_gd(gd->new_gd);
	board_init_f_r_trampoline(gd->start_addr_sp);
#else
	relocate_code(gd->start_addr_sp, gd->new_gd, gd->relocaddr);
#endif

	return 0;
}
#endif

/* Record the board_init_f() bootstage (after arch_cpu_init()) */
static int mark_bootstage(void)
{
	bootstage_mark_name(BOOTSTAGE_ID_START_UBOOT_F, "board_init_f");

	return 0;
}

static int initf_console_record(void)
{
#if defined(CONFIG_CONSOLE_RECORD) && defined(CONFIG_SYS_MALLOC_F_LEN)
	return console_record_init();
#else
	return 0;
#endif
}

static int initf_dm(void)
{
#if defined(CONFIG_DM) && defined(CONFIG_SYS_MALLOC_F_LEN)
	int ret;

	ret = dm_init_and_scan(true);
	if (ret)
		return ret;
#endif
#ifdef CONFIG_TIMER_EARLY
	ret = dm_timer_init();
	if (ret)
		return ret;
#endif

	return 0;
}

/* Architecture-specific memory reservation */
__weak int reserve_arch(void)
{
	return 0;
}

__weak int arch_cpu_init_dm(void)
{
	return 0;
}

static init_fnc_t init_sequence_f[] = {
#ifdef CONFIG_SANDBOX
	setup_ram_buf,
#endif
	/* 函数设置 gd的 mon_len成员变量，即uboot代码的总长度 */
	setup_mon_len,
#ifdef CONFIG_OF_CONTROL
	fdtdec_setup,
#endif
#ifdef CONFIG_TRACE
	trace_early_init,
#endif
	/* 
	 * 函数初始化 gd中跟 malloc有关的成员变量，比如 malloc_limit，
	 * 此函数会设置 gd->malloc_limit = CONFIG_SYS_MALLOC_F_LEN 
	 */
	initf_malloc,
	initf_console_record,
#if defined(CONFIG_MPC85xx) || defined(CONFIG_MPC86xx)
	/* TODO: can this go into arch_cpu_init()? */
	probecpu,
#endif
#if defined(CONFIG_X86) && defined(CONFIG_HAVE_FSP)
	x86_fsp_init,
#endif
	arch_cpu_init,							/* 执行与特定 CPU 架构相关的初始化和配置工作 */
	initf_dm,								/* 驱动模型的一些初始化 */
	arch_cpu_init_dm,
	mark_bootstage,							/* need timer, go after init dm */
#if defined(CONFIG_BOARD_EARLY_INIT_F)
	board_early_init_f, 					/* 板子相关的早期的一些初始化设置，I.MX6ULL用来初始化串口的 IO配置*/
#endif
	/* TODO: can any of this go into arch_cpu_init()? */
#if defined(CONFIG_PPC) && !defined(CONFIG_8xx_CPUCLK_DEFAULT)
	get_clocks,		/* get CPU and bus clocks (etc.) */
#if defined(CONFIG_TQM8xxL) && !defined(CONFIG_TQM866M) \
		&& !defined(CONFIG_TQM885D)
	adjust_sdram_tbs_8xx,
#endif
	/* TODO: can we rename this to timer_init()? */
	init_timebase,
#endif
#if defined(CONFIG_ARM) || defined(CONFIG_MIPS) || \
		defined(CONFIG_BLACKFIN) || defined(CONFIG_NDS32) || \
		defined(CONFIG_SPARC)
	timer_init,		    					/* Cortex-A7内核有一个定时器，这里初始化的就是 Cortex-A内核的那个定时器。 */
#endif
#ifdef CONFIG_SYS_ALLOC_DPRAM
#if !defined(CONFIG_CPM2)
	dpram_init,
#endif
#endif
#if defined(CONFIG_BOARD_POSTCLK_INIT)
	board_postclk_init, 					/* 对于 I.MX6ULL来说是设置 VDDSOC电压 */
#endif
#if defined(CONFIG_SYS_FSL_CLK) || defined(CONFIG_M68K)
	get_clocks,         					/* 函数用于获取一些时钟值， I.MX6ULL获取的是 sdhc_clk时钟，也就是 SD卡外设的时钟 */
#endif
	env_init,		    					/* 函数是和环境变量有关的，设置 gd的成员变量 env_addr，也就是环境变量的保存地址。 */
#if defined(CONFIG_8xx_CPUCLK_DEFAULT)
	/* get CPU and bus clocks according to the environment variable */
	get_clocks_866,
	/* adjust sdram refresh rate according to the new clock */
	sdram_adjust_866,
	init_timebase,
#endif
	init_baud_rate,							/* 函数用于初始化波特率，根据环境变量 baudrate来初始化 gd->baudrate */
	serial_init,							/* 初始化串口 */
	console_init_f,							/* 设置 gd->have_console 为 1，表示有个控制台，此函数也将前面暂存在缓冲区中的数据通过控制台打印出来。*/
#ifdef CONFIG_SANDBOX
	sandbox_early_getopt_check,
#endif
#ifdef CONFIG_OF_CONTROL
	fdtdec_prepare_fdt,
#endif
	display_options,						/* 通过串口输出一些信息 */
	display_text_info,						/* 打印一些文本信息，如果开启 UBOOT的 DEBUG功能的话就会输出 text_base、 bss_start、 bss_end */
#if defined(CONFIG_MPC8260)
	prt_8260_rsr,
	prt_8260_clks,
#endif /* CONFIG_MPC8260 */
#if defined(CONFIG_MPC83xx)
	prt_83xx_rsr,
#endif
#if defined(CONFIG_PPC) || defined(CONFIG_M68K)
	checkcpu,
#endif
	print_cpuinfo,									/* 函数用于打印 CPU信息 (以及cpu速度 speed) */
#if defined(CONFIG_MPC5xxx)
	prt_mpc5xxx_clks,
#endif /* CONFIG_MPC5xxx */
#if defined(CONFIG_DISPLAY_BOARDINFO)
	show_board_info,    							/* 函数用于打印板子信息，会调用checkboard函数 */
#endif
	INIT_FUNC_WATCHDOG_INIT 						/* 初始化看门狗 */
#if defined(CONFIG_MISC_INIT_F)
	misc_init_f,
#endif
	INIT_FUNC_WATCHDOG_RESET						/* 复位看门狗 */
#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SYS_I2C)
	init_func_i2c,									/* 初始化 I2C */
#endif
#if defined(CONFIG_HARD_SPI)
	init_func_spi,								    /* 初始化 SPI */
#endif
	announce_dram_init,								/* 就是输出字符串"DRAM:" */
	/* TODO: unify all these dram functions? */
#if defined(CONFIG_ARM) || defined(CONFIG_X86) || defined(CONFIG_NDS32) || \
		defined(CONFIG_MICROBLAZE) || defined(CONFIG_AVR32)
	dram_init,										/* 并非真正的初始化 DDR，只是设置 gd->ram_size的值 */
#endif
#if defined(CONFIG_MIPS) || defined(CONFIG_PPC) || defined(CONFIG_M68K)
	init_func_ram,
#endif
#ifdef CONFIG_POST
	post_init_f,
#endif
	INIT_FUNC_WATCHDOG_RESET						/* 复位看门狗 */
#if defined(CONFIG_SYS_DRAM_TEST)
	testdram,
#endif /* CONFIG_SYS_DRAM_TEST */
	INIT_FUNC_WATCHDOG_RESET

#ifdef CONFIG_POST
	init_post,
#endif
	INIT_FUNC_WATCHDOG_RESET
	/*
	* 现在我们已经映射并使 DRAM 工作，我们可以
	* 重定位代码并继续从 DRAM 运行。
	*
	* 在 RAM 结尾保留内存（自顶向下的顺序）：
	*  - 不会被 U-Boot 和 Linux 使用的区域（可选）
	*  - 内核日志缓冲区
	*  - 受保护的 RAM
	*  - LCD 帧缓冲区
	*  - 监视器代码
	*  - board info结构体
	*/
	setup_dest_addr,				/* 设置目的地址，设置 gd->ram_size gd->ram_top gd->relocaddr这三个的值 */
#if defined(CONFIG_BLACKFIN)
	/* Blackfin u-boot monitor should be on top of the ram */
	reserve_uboot,
#endif
#if defined(CONFIG_SPARC)
	reserve_prom,
#endif
#if defined(CONFIG_LOGBUFFER) && !defined(CONFIG_ALT_LB_ADDR)
	reserve_logbuffer,
#endif
#ifdef CONFIG_PRAM
	reserve_pram,
#endif
	reserve_round_4k,				/* 函数用于对 gd->relocaddr做 4KB对齐 */
#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF)) && \
		defined(CONFIG_ARM)
	reserve_mmu,					/* 留出 MMU的 TLB表的位置，分配 MMU的 TLB表内存以后会对 gd->relocaddr做 64K字节对齐 */
#endif
#ifdef CONFIG_DM_VIDEO
	reserve_video,
#else
# ifdef CONFIG_LCD
	reserve_lcd,
# endif
	/* TODO: Why the dependency on CONFIG_8xx? */
# if defined(CONFIG_VIDEO) && (!defined(CONFIG_PPC) || defined(CONFIG_8xx)) && \
		!defined(CONFIG_ARM) && !defined(CONFIG_X86) && \
		!defined(CONFIG_BLACKFIN) && !defined(CONFIG_M68K)
	reserve_legacy_video,
# endif
#endif /* CONFIG_DM_VIDEO */
	reserve_trace,
#if !defined(CONFIG_BLACKFIN)
	/*
	* 留出重定位后的 uboot所占用的内存区域， uboot所占用大小由gd->mon_len所指定，
	* 留出 uboot的空间以后还要对 gd->relocaddr做 4K字节对齐，并且重新设置 gd->start_addr_sp
	*/
	reserve_uboot,                 
#endif
#ifndef CONFIG_SPL_BUILD
	reserve_malloc,			/* 留出 malloc区域，调整 gd->start_addr_sp位置， malloc区域由宏TOTAL_MALLOC_LEN定义 */
	reserve_board, 			/* 留出板子 bd所占的内存区， bd是结 构体 bd_t bd_t大小为80字节，结果如图 */
#endif
	setup_machine, 			/* 设置机器 ID 不用这种方式了，这是以前老版本的 uboot和linux使用的，新版本使用设备树了，因此此函数无效。*/
	reserve_global_data, 	/* 保留出 gd_t的内存区域 */
	reserve_fdt,   			/* 留出设备树相关的内存区域 */
	reserve_arch,
	/*
	 * 留出栈空间，先对 gd->start_addr_sp减去 16，然后做 16字节对齐，
	 * 如果使能IRQ的话还要留出 IRQ相应栈空间，具体工作是由 arch/arm/lib/stack.c文件中的
     * 函数 arch_reserve_stacks完成
	 */
	reserve_stacks,
	/*
	 * 设置 dram信息，就是设置 gd->bd->bi_dram[0].start和gd->bd->bi_dram[0].size，
	 * 后面会传递给 linux内核，告诉 linux DRAM的起始地址和大小
	 */
	setup_dram_config,
	show_dram_config,  /*用于显示 DRAM的配置 */
#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_MIPS)
	setup_board_part1,
#endif
#if defined(CONFIG_PPC) || defined(CONFIG_M68K)
	INIT_FUNC_WATCHDOG_RESET
	setup_board_part2,
#endif
	display_new_sp,   /*显示新的 sp位置*/
#ifdef CONFIG_SYS_EXTBDINFO
	setup_board_extra,
#endif
	INIT_FUNC_WATCHDOG_RESET
	reloc_fdt,        /* 函数用于重定位 fdt */
	/* 
	* 设置 gd的其他一些成员变量，供后面重定位的时候使用，并且将以前的 gd拷贝到 gd->new_gd处。
	* 需要使能 DEBUG才能看到相应的信息输出
	*/
	setup_reloc,
#if defined(CONFIG_X86) || defined(CONFIG_ARC)
	copy_uboot_to_ram,
	clear_bss,
	do_elf_reloc_fixups,
#endif
#if !defined(CONFIG_ARM) && !defined(CONFIG_SANDBOX)
	jump_to_copy,
#endif
	NULL,
};

void board_init_f(ulong boot_flags)
{
#ifdef CONFIG_SYS_GENERIC_GLOBAL_DATA
	/*
	 * For some archtectures, global data is initialized and used before
	 * calling this function. The data should be preserved. For others,
	 * CONFIG_SYS_GENERIC_GLOBAL_DATA should be defined and use the stack
	 * here to host global data until relocation.
	 */
	gd_t data;

	gd = &data;

	/*
	 * Clear global data before it is accessed at debug print
	 * in initcall_run_list. Otherwise the debug print probably
	 * get the wrong vaule of gd->have_console.
	 */
	zero_global_data();
#endif

	gd->flags = boot_flags;
	gd->have_console = 0;

	if (initcall_run_list(init_sequence_f))
		hang();

#if !defined(CONFIG_ARM) && !defined(CONFIG_SANDBOX) && \
		!defined(CONFIG_EFI_APP)
	/* NOTREACHED - jump_to_copy() does not return */
	hang();
#endif
}

#if defined(CONFIG_X86) || defined(CONFIG_ARC)
/*
 * For now this code is only used on x86.
 *
 * init_sequence_f_r is the list of init functions which are run when
 * U-Boot is executing from Flash with a semi-limited 'C' environment.
 * The following limitations must be considered when implementing an
 * '_f_r' function:
 *  - 'static' variables are read-only
 *  - Global Data (gd->xxx) is read/write
 *
 * The '_f_r' sequence must, as a minimum, copy U-Boot to RAM (if
 * supported).  It _should_, if possible, copy global data to RAM and
 * initialise the CPU caches (to speed up the relocation process)
 *
 * NOTE: At present only x86 uses this route, but it is intended that
 * all archs will move to this when generic relocation is implemented.
 */
static init_fnc_t init_sequence_f_r[] = {
	init_cache_f_r,

	NULL,
};

void board_init_f_r(void)
{
	if (initcall_run_list(init_sequence_f_r))
		hang();

	/*
	 * U-Boot has been copied into SDRAM, the BSS has been cleared etc.
	 * Transfer execution from Flash to RAM by calculating the address
	 * of the in-RAM copy of board_init_r() and calling it
	 */
	(board_init_r + gd->reloc_off)((gd_t *)gd, gd->relocaddr);

	/* NOTREACHED - board_init_r() does not return */
	hang();
}
#endif /* CONFIG_X86 */
