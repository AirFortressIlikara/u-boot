/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __LOONGSON_ENV_H__
#define __LOONGSON_ENV_H__


#define CMDLINE_CONSOLE		"console=ttyS0,115200"

#ifdef CONFIG_64BIT
#define FDT_ADDR    0x900000000a000000

#if defined(CONFIG_SOC_LS2K300)
#define RD_ADDR     0x9000000007000000
#endif

#if RD_ADDR >= LOCK_CACHE_BASE && RD_ADDR < (LOCK_CACHE_BASE + LOCK_CACHE_SIZE)
#error should adjust RD_ADDR because conflict with LOCK_CACHE_BASE and SIZE (asm/addrspace.h)
#endif

#else
#define FDT_ADDR    0x0a000000
#define RD_ADDR     0x07000000
#endif

#define RD_SIZE		0x2000000 /* ramdisk size:32M == 32768K*/

/*dtb size: 56K*/
#define FDT_SIZE	0xE000

#define RECOVER_FRONT_BOOTARGS "setenv bootargs " CMDLINE_CONSOLE " \
mtdparts=${mtdparts} root=/dev/ram0 init=/linuxrc rw rootfstype=ext2 fbcon=rotate:${rotate} panel=${panel};"

#define RECOVER_START "bootm ${loadaddr}"

#define RECOVER_TFTP_DOWNLOAD_CMD "tftpboot ${loadaddr} uImage;tftpboot ${rd_start} ramdisk.gz;"

#define RECOVER_DHCP_DOWNLOAD_CMD "dhcp ${loadaddr} uImage;dhcp ${rd_start} ramdisk.gz;"

#define EMMC_BOOT_ENV "setenv bootargs " CMDLINE_CONSOLE " noinitrd init=/sbin/init rootfstype=ext4 rw rootwait; \
setenv bootcmd ' setenv bootargs ${bootargs} root=/dev/mmcblk0p${syspart} mtdparts=${mtdparts} fbcon=rotate:${rotate} panel=${panel}; \
ext4load mmc 0:1 ${loadaddr} /boot/uImage;bootm ';\
saveenv;"

#define BOOT_EMMC_DEFAULT EMMC_BOOT_ENV"boot"

#define SDCARD_BOOT_ENV "setenv bootargs " CMDLINE_CONSOLE " noinitrd init=/sbin/init  rootfstype=ext4 rw rootwait; \
setenv bootcmd ' setenv bootargs ${bootargs} root=/dev/mmcblk1p${syspart} mtdparts=${mtdparts} fbcon=rotate:${rotate} panel=${panel}; \
ext4load mmc 1:1 ${loadaddr} /boot/uImage;bootm ';\
saveenv;"

#define BOOT_SDCARD_DEFAULT SDCARD_BOOT_ENV"boot"

#endif /* __LOONGSON_ENV_H__ */
