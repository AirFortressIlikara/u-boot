// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <malloc.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <env.h>

#include <jffs2/jffs2.h>

#include "loongson_update.h"

extern int mtdparts_init(void);

DECLARE_GLOBAL_DATA_PTR;

const char *update_devname_str[UPDATE_DEV_COUNT] = {
	[UPDATE_DEV_USB]	= "usb",
	[UPDATE_DEV_TFTP]	= "tftp",
	[UPDATE_DEV_MMC]	= "mmc",
	[UPDATE_DEV_DHCP]	= "dhcp",
};

const char *update_typename_str[UPDATE_TYPE_COUNT] = {
	[UPDATE_TYPE_SYSTEM]	= "system",
};
