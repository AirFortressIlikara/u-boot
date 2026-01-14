#
# (C) Copyright 2019
# <tang-haifeng@foxmail.com> <pengren.mcu@qq.com>
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-$(CONFIG_TARGET_LOONGSON_2K)		+= platform.o

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_TARGET_LOONGSON_2K)		+= loongson_update.o loongson_storage_read_file.o
obj-$(CONFIG_LOONGSON_RECOVER)		+= recover.o
obj-y += bootparam.o loongson_efi_systab.o

ifeq ($(CONFIG_MENU), y)
obj-$(CONFIG_TARGET_LOONGSON_2K) += loongson_update_menu.o
endif

endif
