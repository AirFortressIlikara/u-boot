// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <malloc.h>

#include "loongson_boot.h"
#include "bdinfo/bdinfo.h"

typedef int (*loongson_boot_func)(void);

static int loongson_boot_emmc(void)
{
#ifdef BOOT_EMMC_DEFAULT
	printf("boot system from emmc .....\r\n");
	return run_command(BOOT_EMMC_DEFAULT, 0);
#else
	return -1;
#endif
}

static int loongson_boot_sdcard(void)
{
#ifdef BOOT_SDCARD_DEFAULT
	printf("boot system from sdcard .....\r\n");
	return run_command(BOOT_SDCARD_DEFAULT, 0);
#else
	return -1;
#endif
}

static char* boot_param_list[LOONGSON_BOOT_TYPE_SIZE] = {"emmc", "sdcard"};
static loongson_boot_func boot_func_list[LOONGSON_BOOT_TYPE_SIZE] = {loongson_boot_emmc, loongson_boot_sdcard};

static int do_loongson_boot(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	int index;

	if(argc != 2)
		return ret;

	if(!argv[1])
		return ret;

	index = 0;
	for (index = 0; index < LOONGSON_BOOT_TYPE_SIZE; ++index) {
		if(!strcmp(argv[1], boot_param_list[index])) {
			ret = (boot_func_list[index])();
			break;
		}
	}

	return ret;
}

#define LOONGSON_BOOT_HELP_HEAD "set system boot from where."

#define LOONGSON_BOOT_USAGE_HEAD "<option>\n" \

#define LOONGSON_BOOT_EMMC_USAGE "option: emmc: boot from emmc\n"

#define LOONGSON_BOOT_SDCARD_USAGE "option: sdcard: boot from sdcard\n"

#define LOONGSON_BOOT_HELP LOONGSON_BOOT_HELP_HEAD

#define LOONGSON_BOOT_USAGE LOONGSON_BOOT_USAGE_HEAD \
							LOONGSON_BOOT_EMMC_USAGE \
							LOONGSON_BOOT_SDCARD_USAGE

U_BOOT_CMD(
	loongson_boot,    2,    1,     do_loongson_boot,
	LOONGSON_BOOT_HELP,
	LOONGSON_BOOT_USAGE
);
