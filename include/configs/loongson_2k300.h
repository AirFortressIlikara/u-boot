/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * DEV_LS2K300 configuration
 *
 * Copyright (c) 2022 Loongson Technologies
 * Author: Xiaochuan Mao<maoxiaochuan@loongson.cn>
 */

#ifndef __LOONGSON_LA_COMMON_H__
#define __LOONGSON_LA_COMMON_H__

#include <linux/sizes.h>
#include "loongson_common.h"

/* Loongson LS2K300 clock configuration. */
#define REF_FREQ				120		//参考时钟固定为120MHz
#define CORE_FREQ				CONFIG_CPU_FREQ	//现在CPU的时钟在 make menuconfig 里面选择
#define DDR_FREQ				800		//MEM 800Mhz
#define APB_FREQ				200		//SB 100~200MHz, for BOOT, USB, APB, SDIO
#define NET_FREQ				200		//NETWORK 200~400MHz, for NETWORK, DC


/* Memory configuration */
#define CONFIG_SYS_BOOTPARAMS_LEN	SZ_64K
#define CONFIG_SYS_MONITOR_BASE		CONFIG_TEXT_BASE

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SPL_STACK			0x9000000090040000
#endif

/* UART configuration */
#define CONSOLE_BASE_ADDR			LS_UART0_REG_BASE
/* NS16550-ish UARTs */
#define CONFIG_SYS_NS16550_CLK		(APB_FREQ * 1000000)	// CLK_in: 100MHz

/* Environment settings */
// #define CONFIG_ENV_SIZE			0x4000	/* 16KB */
#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
// #define CONFIG_ENV_SIZE                 0x4000  /* 16KB */

/* video configuration */
// #define DISPLAY_BANNER_ON_VIDCONSOLE

#define DBG_ASM

#endif /* __LOONGSON_LA_COMMON_H__ */
