// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <stdlib.h>
#include "cmd_gl_arg.h"
#include "general_load.h"

// [--from xxx [--fs xxx]] [--to xxx [--fs xxx]]
static int do_general_load(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	gl_target_t* src;
	gl_target_t* dest;
	enum gl_extra_e extra = GL_EXTRA_NONE;

	if(argc == 1)
		goto end;

	src = new_gl_target();
	if (src == NULL)
		goto end;
	dest = new_gl_target();
	if (dest == NULL)
		goto free_src;


	if (cmd_gl_parse((char**)argv + 1, argc - 1, src, dest, &extra))
		goto free_targets;

	ret = general_load(src, dest, extra);

free_targets:
	destroy_gl_target(dest);
free_src:
	destroy_gl_target(src);
end:
	return ret;
}

#define GENERAL_LOAD_CMD_TIP "Load file from device to device.\n"\
	"general_load [--if <device> [--fmt <ext4/tftp> --sym <path/name>]] [--of <device> [--fmt <ext4> --sym <path/name>]] [--decompress] [--force]\n"\
	"general_load --if net:192.168.1.2 --fmt tftp --sym hello.txt --of mmc0:1 --fmt ext4 --sym /root/hello.txt\n"\
	"general_load --if net --sym rootfs.img --decompress\n"\
	"general_load --if usb --sym uImage\n"


// load [--if <device> [--fmt <ext4/tftp> --sym <path/name>]] [--of <device> [--fmt <ext4> --sym <path/name>]] [--decompress]
//	--if	设备，例如 usb0:1 mmc0 net
// 	--fmt	不指定，默认纯数据读写
// 	--sym	文件路径
// 	--decompress	gunzip 解压
// 	--force	不进行语义解析
// 		general_load --if usb0:1 --fmt fat --sym /uImage --of ram --force
U_BOOT_CMD(
	general_load,    14,    0,     do_general_load,
	"general load from xxx to xxx",
	GENERAL_LOAD_CMD_TIP
);


