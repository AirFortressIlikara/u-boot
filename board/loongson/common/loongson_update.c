// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/cache.h>
#include <asm/global_data.h>

#include <jffs2/jffs2.h>

#include "loongson_update.h"
#include "bdinfo/bdinfo.h"
#include "loongson_storage_read_file.h"

extern int mtdparts_init(void);

DECLARE_GLOBAL_DATA_PTR;

const char *update_devname_str[UPDATE_DEV_COUNT] = {
	[UPDATE_DEV_USB]	= "usb",
	[UPDATE_DEV_TFTP]	= "tftp",
	[UPDATE_DEV_MMC]	= "mmc",
	[UPDATE_DEV_DHCP]	= "dhcp",
};

const char *update_typename_str[UPDATE_TYPE_COUNT] = {
	[UPDATE_TYPE_KERNEL]	= "kernel",
	[UPDATE_TYPE_ROOTFS]	= "rootfs",
	[UPDATE_TYPE_UBOOT]		= "uboot",
	[UPDATE_TYPE_DTB]		= "dtb",
	[UPDATE_TYPE_SYSTEM]	= "system",
	[UPDATE_TYPE_BOOTSELECT]		= "bootselect",
	[UPDATE_TYPE_RESOLUTION]		= "resolution",
};

static void user_env_save(void)
{
	char *env_val;
	char *bdi_val;
	bool need_save = false;

	// syspart 用于保存操作系统分区的分区号，如：
	//   linux: sda1、sda2 ...
	//   uboot: mmc 0:1、mmc 0:2 ...
	env_val = env_get("syspart");
	if (env_val == NULL)
		return;

	bdi_val = bdinfo_get(BDI_ID_SYSPART);

	if (strcmp(env_val, bdi_val) != 0) {
		bdinfo_set(BDI_ID_SYSPART, env_val);
		need_save = true;
	}

	if (need_save)
		bdinfo_save();
}

static void user_env_fetch_about_syspart(void)
{
	char *bdi_val;
	char *temp_env_val;

	// syspart 这个变量被人为改动 所以不需要按bdiinfo的信息为准
	// 需要同步到 bdiinfo才真
	temp_env_val = env_get("syspart_ch");
	if (temp_env_val && !strcmp(temp_env_val, "1")) {
		env_set("syspart_ch", "0");
		env_save();
		user_env_save();
		return;
	}

	bdi_val = bdinfo_get(BDI_ID_SYSPART);
	if (bdi_val)
		env_set("syspart", bdi_val);
}

void user_env_fetch(void)
{
	if(bdinfo_save_in_nv()) {
		user_env_save();
		return;
	}
	user_env_fetch_about_syspart();
}

static void update_failed_way_tip(int way)
{
	if (way == UPDATE_DEV_USB) {
		printf("### check usb filesystem type must be fat32 or ext4\n");
	} else if (way == UPDATE_DEV_TFTP) {
		printf("### check tftp server running in PC\n");
		printf("### check board link PC by eth cable(RJ45)\n");
		printf("### check uboot env ipaddr and serverip(PC ip)\n");
	} else if (way == UPDATE_DEV_MMC) {
		printf("### check sd card filesystem type must be fat32\n");
	} else if (way == UPDATE_DEV_DHCP) {
		printf("### check tftp server running in PC\n");
		printf("### check board link PC by eth cable(RJ45)\n");
		printf("### check uboot env serverip(PC ip)\n");
		printf("### check board and PC link router(not PC link board)\n");
	}
}

static void update_failed_target_tip(int way, int target)
{
	char uboot_file[] = "u-boot-with-spl.bin";
	char dtb_file[] = "dtb.bin";
	char* file;

	if (target == UPDATE_TYPE_UBOOT)
		file = uboot_file;
	else if (target == UPDATE_TYPE_DTB)
		file = dtb_file;

	if (way == UPDATE_DEV_USB) {
		printf("### ensure %s in update dir(usb)\n", file);
	} else if (way == UPDATE_DEV_TFTP) {
		printf("### ensure %s in tftp server dir\n", file);
	} else if (way == UPDATE_DEV_MMC) {
		printf("### ensure %s in update dir(sd card)\n", file);
	} else if (way == UPDATE_DEV_DHCP) {
		printf("### ensure %s in tftp server dir\n", file);
	}
}

static void update_result_display(int res, int way, int target)
{
	printf("\n");

	printf("######################################################\n");
	printf("### update target: %s\n", update_typename_str[target]);
	printf("### update way   : %s\n", update_devname_str[way]);
	printf("### update result: %s\n", res ? "failed" : "success");


	if (res) {
		printf("###\n");
		printf("### tip:\n");
		update_failed_way_tip(target);
		update_failed_target_tip(way, target);
	}

	printf("######################################################\n\n");
}

static int update_uboot(int dev)
{
	int ret = -1;
	char cmd[256];
	char *image_name[] = {
		"u-boot-with-spl.bin",
		"u-boot.bin"
	};

	printf("update u-boot.............\n");

	for (int i = 0; i < sizeof(image_name)/sizeof(image_name[0]); i++) {
		printf("try to get %s ......\n", image_name[i]);
		memset(cmd, 0, 256);
		switch (dev) {
		case UPDATE_DEV_USB:
			sprintf(cmd, "/update/%s", image_name[i]);
			ret = storage_read_file(IF_TYPE_USB, "${loadaddr}", cmd, 0, NULL, NULL);
			break;
		case UPDATE_DEV_TFTP:
			sprintf(cmd, "tftpboot ${loadaddr} %s", image_name[i]);
			ret = run_command(cmd, 0);
			break;
		case UPDATE_DEV_MMC:
			sprintf(cmd, "/update/%s", image_name[i]);
			ret = storage_read_file(IF_TYPE_MMC, "${loadaddr}", cmd, 0, NULL, NULL);
			break;
		case UPDATE_DEV_DHCP:
			sprintf(cmd, "dhcp ${loadaddr} ${serverip}:%s", image_name[i]);
			ret = run_command(cmd, 0);
			break;
		}

		if (!ret)
			break;
	}

	if (ret) {
		if (ret) {
			printf("upgrade uboot failed, not found u-boot image!\r\n");
			return ret;
		}
	}

	if (run_command("sf probe", 0) == 0) {
		char buf[128] = {0};
		// snprintf(buf, sizeof(buf), "mtd erase dtb");
		// ret = run_command(buf, 0);
		snprintf(buf, sizeof(buf), "sf erase 0 0x%x;sf update ${loadaddr} 0 ${filesize}", CONFIG_ENV_OFFSET);
		ret = run_command(buf, 0);
	} else {
		struct mtd_device *mtd_dev;
		struct part_info *part;
		u8 pnum;
		ulong uboot_size;

		find_dev_and_part("uboot", &mtd_dev, &pnum, &part);
		uboot_size = part->size;

		// erase operation maybe take long time, show the informations.
		printf("Erase uboot partition ... ");
		sprintf(cmd, "sf erase uboot %lx", uboot_size);
		ret = run_command(cmd, 0);
		if (ret)
			goto out;

		sprintf(cmd, "sf update ${loadaddr} uboot ${filesize}");
		ret = run_command(cmd, 0);
		if (ret)
			goto out;
	}

	/*
	 * erase uboot_env
	 */
	printf("Erase uboot_env partition ... ");
	sprintf(cmd, "mtd erase uboot_env");
	run_command(cmd, 0);

	/*
	 * erase u-boot ddr param
	 */
	printf("Erase ddr_context partition ... ");
	sprintf(cmd, "mtd erase ddr_context");
	run_command(cmd, 0);

	user_env_save();

out:
	update_result_display(ret, dev, UPDATE_TYPE_UBOOT);

	return ret;
}

static int update_dtb(int dev)
{
	int ret = -1;

	printf("update dtb.............\n");

	switch (dev) {
	case UPDATE_DEV_USB:
		ret = storage_read_file(IF_TYPE_USB, "${loadaddr}", "/update/dtb.bin", 0, NULL, NULL);
		break;
	case UPDATE_DEV_TFTP:
		ret = run_command("tftpboot ${loadaddr} dtb.bin", 0);
		break;
	case UPDATE_DEV_MMC:
		ret = storage_read_file(IF_TYPE_MMC, "${loadaddr}", "/update/dtb.bin", 0, NULL, NULL);
		break;
	}

	if (ret) {
		if (ret) {
			printf("upgrade dtb failed, not found /update/dtb.bin file!\r\n");
			return ret;
		}
	}

	if (run_command("sf probe", 0)) {
		ret = -1;
	} else {
		char buf[128] = {0};
		snprintf(buf, sizeof(buf), "sf erase dtb 0x%x;sf update ${loadaddr} dtb ${filesize}", FDT_SIZE);
		ret = run_command(buf, 0);
	}

	update_result_display(ret, dev, UPDATE_TYPE_DTB);

	return ret;
}

static int do_loongson_update(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
	int dev = UPDATE_DEV_UNKNOWN;
	int type = UPDATE_TYPE_UNKNOWN;

	if (!argv[1] || !argv[2]) {
		printf("\n");
		printf("######################################################\n");
		printf("### error! loongson_update need more param\n");
		goto err;
	}

	for (dev = 0; dev < UPDATE_DEV_COUNT; dev++) {
		if (update_devname_str[dev] &&
			!strcmp(argv[1], update_devname_str[dev]))
			break;
	}
	if ((dev == UPDATE_DEV_UNKNOWN) || (dev >= UPDATE_DEV_COUNT)) {
		printf("\n");
		printf("######################################################\n");
		printf("### upgrade dev unknown\r\n");
		printf("### please intput dev: usb\\tftp\\mmc\\dhcp\r\n");
		goto err;
	}

	for (type = 0; type < UPDATE_TYPE_COUNT; type++) {
		if (update_typename_str[type] &&
			!strcmp(argv[2], update_typename_str[type]))
			break;
	}
	if ((type == UPDATE_TYPE_UNKNOWN) || (type >= UPDATE_TYPE_COUNT)) {
		printf("\n");
		printf("######################################################\n");
		printf("### upgrade type unknown\r\n");
		printf("### please intput type: uboot\\dtb\r\n");
		goto err;
	}

	switch (dev) {
	case UPDATE_DEV_USB:
		run_command("usb reset", 0);
		ret = run_command("usb storage", 0);
		if (ret) {
			printf("### upgrade failed no usb storage!\r\n");
			goto err;
		}
		break;
	case UPDATE_DEV_TFTP:
		break;
	case UPDATE_DEV_MMC:
		ret = run_command("mmc rescan", 0);
		if (ret) {
			printf("### upgrade failed no mmc storage!\r\n");
			goto err;
		}
		break;
	case UPDATE_DEV_DHCP:
		break;
	default:
		break;
	}

	switch (type) {
	case UPDATE_TYPE_UBOOT:
		ret = update_uboot(dev);
		break;
	case UPDATE_TYPE_DTB:
		ret = update_dtb(dev);
		break;
	default:
		break;
	}

	return ret;

err:
	printf("######################################################\n");
	return -1;
}

U_BOOT_CMD(
	loongson_update,    5,    1,     do_loongson_update,
	"upgrade uboot\\dtb over usb\\tftp\\mmc\\dhcp.",
	""
);

