#include <config.h>
#include <linux/linkage.h>
#include <asm/regdef.h>
#include <asm/addrspace.h>
#include <mach/loongson.h>

#ifndef CONFIG_SYS_INIT_SP_ADDR
#define CONFIG_SYS_INIT_SP_ADDR		PHYS_TO_CACHED(0x02000000)	// 32MB
#endif

// Note: the Address mapping window NOT setup yet.
// 		 so, use the real physicl address.

ENTRY(lowlevel_init)
    or     a4, ra, zero

	// CPUCFG 0x4 CC_FREQ 恒定频率计时器和定时器所用时钟对应的晶振频率 0x0727 0E00 (120,000,000Hz = 120MHz)
	// CPUCFG 0x5 CC_MUL 恒定频率计时器和定时器所用时钟对应的倍频因子 0x1
	// CPUCFG 0x5 CC_DIV 恒定频率计时器和定时器所用时钟对应的分频系数 0x1
	// 修复参考时钟计算错误的问题（可能默认值为 100MHz?）
	li.d	t0, 0x0001000107270e00
	csrwr	t0, LOONGARCH_CSR_MCSR2

	// 芯片通用配置寄存器 00 : ddr4_regs_default 窗口不命中处理 1：当访问内存控制器所有窗口不命中时，由内存配置空间默认给出响应，防止内存访问卡死
	li.d	t0, LS_GENERAL_CFG0
	ld.w	t1, t0, 0x0
	li.w	t2, (1 << 5)
	or	t1, t1, t2
	st.w	t1, t0, 0x0

	// SPI0 0x4 SFC_PARAM clk_div=4 
	// fast_read = 1 快速读模式
	// burst_en = 1 SPI flash 支持连续地址读模式
	// memory_en = 1 SPI flash 读使能，无效时 csn[0]可由软件控制。
	li.d	t0, LS_SPI0_BASE
	li.w	t1, 0x47
	st.b	t1, t0, 0x4

//#define SPI_QUAD_IO
# #ifdef	SPI_QUAD_IO
# 	/* spi quad_io */
# 	li.w	t1, 0xb
# 	st.b	t1, t0, 0x6
# spi_quad_io:
# 	ld.bu	t2, t0, 0x6
# 	bne	t2, t1, spi_quad_io
# #endif

	// 手册里没看到
	/*close usb & ddr soft reset: [0] ddr, [7] usbm*/
	li.w  	t1, 0xffffff7e
	st.w 	t1, t0, 0x1c

	/*unvalid ddr soft reset: [0] ddr*/
	ld.w	t1, t0, 0x1c
	ori     t1, t1, 0x1
	st.w 	t1, t0, 0x1c

	// apb设备DMA/usb/gmac0/gmac1/dc 使能设备CACHE访问配置有效 + 开启CACHE加速访问
	/*[103:96] en [111:104] coherent*/
	/*0:cpu    1:usb     2:gmac0    3:gmac1   4:dc
		5:apb-dma(sdio spi can .etc)*/
	li.d	t0, LS_GENERAL_CFG3
	ld.w	t1, t0, 0x0
	li.w	t2, ((0x1f << 1) | (0x1f << 9))  //cache coherent for usb/gmac0/gmac1/dc
	or	t1, t1, t2
	st.w	t1, t0, 0x0

	/* select uart0 */
	/*cfg pins to main, uart0 use GPIO 40/41*/
	li.d	t0, LS_GPIO_32_47_MULTI_CFG
	li.w	t1, 0
	li.w	t2, 0xf0000
	st.w	t2, t0, 0x0

	#include "clksetting_2k300.S"

	//delay a while
	li.d	t1, 0x100
1:
	addi.d	t1, t1, -1
	bnez	t1, 1b
	nop

	or	ra, a4, zero
	jirl	zero, ra, 0
ENDPROC(lowlevel_init)


// Note: the Address mapping window setting up already.
// 		 so, use the Mapped address 0x8xxx... or 0x9xxx...
/*
 * Simple character printing routine used before full initialization
 */
#define UART_REF_CLK	(APB_FREQ * 1000000)
#define UART_DIV_HI	(((UART_REF_CLK + (115200*8)) / (115200*16)) >> 8)
#define UART_DIV_LO	(((UART_REF_CLK + (115200*8)) / (115200*16)) & 0xff)
ENTRY(init_serial)
	or     a4, ra, zero

	// UART LCR dlab = 1 分频锁存器访问位 访问操作分频锁存器
	li.d	a0, CONSOLE_BASE_ADDR
	li.w	a1, 0x80
	st.b	a1, a0, 3

	// 写入分频系数
	li.w	a1, UART_DIV_HI
	st.b	a1, a0, 1
	li.w	a1, UART_DIV_LO
	st.b	a1, a0, 0

	// UART LCR dlab = 0   分频锁存器访问位 访问操作正常寄存器
	//          bec = 0b10 设定每个字符的位数=7
	li.w	a1, 3	#CFCR_8BITS
	st.b	a1, a0, 3

	// 这又是啥啊
	li.w	a1, 71
	st.b	a1, a0, 2

	or      ra, a4, zero
	jirl	zero, ra, 0
ENDPROC(init_serial)

ENTRY(ram_init)
	or	s1, ra, zero

#if !defined(CONFIG_SPL) || defined(CONFIG_SPL_BUILD)
	// PRINTSTR("\r\nlock scache for early stack: ")
	// li.d	a0, LOCK_CACHE_BASE
	// bl		printhex64
	// PRINTSTR(" - ")
	// li.d	a0, LOCK_CACHE_BASE + LOCK_CACHE_SIZE
	// bl		printhex64

	// 手册上不存在的寄存器 Lock Scache
	li.d	t0, LS_SCACHE_LOCK_WIN0_BASE
	li.d	t1, ~(LOCK_CACHE_SIZE - 1)
	st.d	t1, t0, 0x40
	li.d	t1, (LOCK_CACHE_BASE & 0xffffffffffff) | (1 << 63)
	st.d	t1, t0, 0x0
#ifdef DBG_ASM
	PRINTSTR("\r\nLock Scache Done.\r\n")
#endif
#endif

#if defined(CONFIG_SPL_BUILD)
#ifdef DBG_ASM
	PRINTSTR("\r\nCopy spl code to locked scache...\r\n");
#endif
	// 循环每 4B 从 SPI0 Boot 地址复制 spl 程序到 __image_copy_start __image_copy_end 之间的 locked scache
	// copy spl code to locked scache
	li.d	t0, BOOT_SPACE_BASE_UNCACHED
	la	t1, __text_start
	la	t2, __image_copy_end
1:
	ld.w	t3, t0, 0
	st.w	t3, t1, 0
	addi.d	t0, t0, 4
	addi.d	t1, t1, 4
	blt	t1, t2, 1b

	// 循环每 4B 将 __bss_start __bss_end 之间的 locked scache 写 0
	// clear bss
	la	t0, __bss_start
	la	t1, __bss_end
2:
	st.w	zero, t0, 0
	addi.d	t0, t0, 4
	blt	t0, t1, 2b

	// jump to cache
	la	t0, jump_cache
	jirl	zero, t0, 0
jump_cache:

	li.d	a0, LOCK_CACHE_BASE + LOCK_CACHE_SIZE

#elif !defined(CONFIG_SPL)
	li.d	t0, LOCK_CACHE_BASE + LOCK_CACHE_SIZE
	or		sp, t0, zero
#ifdef DBG_ASM
	PRINTSTR("\r\nJump to ddr_init.......\r\n");
#endif
	la		t8, ddr_init
	jirl	ra, t8, 0

#ifdef DBG_ASM
	PRINTSTR("\r\nUnlock scache........\r\n")
#endif
	// unlock scache
	li.d	t0, LS_SCACHE_LOCK_WIN0_BASE
	st.d	zero, t0, 0x40
	st.d	zero, t0, 0
	li.d	t0, LOCK_CACHE_BASE
	li.d	t1, LOCK_CACHE_BASE + LOCK_CACHE_SIZE
2:
	cacop	0x13, t0, 0
	addi.d	t0, t0, 0x40
	blt	t0, t1, 2b

	// return the sp addr.
	li.d	a0, CONFIG_SYS_INIT_SP_ADDR
#else
	li.d	a0, CONFIG_SYS_INIT_SP_ADDR
#endif

	or	ra, s1, zero
	jirl	zero, ra, 0
ENDPROC(ram_init)


/******************************************************
 *used: a0 - char to print, a1, a2
 ******************************************************/
ENTRY(printchar)
	li.d	a1, CONSOLE_BASE_ADDR
	// 循环到 TFE 传输 FIFO 位空表示位为 1
1:
	ld.bu	a2, a1, 0x5
	andi	a2, a2, 0x20
	beqz	a2, 1b

	st.b	a0, a1, 0

	jirl	zero, ra, 0
ENDPROC(printchar)
