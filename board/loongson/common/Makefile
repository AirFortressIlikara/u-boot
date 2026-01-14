#
# (C) Copyright 2019
# <tang-haifeng@foxmail.com> <pengren.mcu@qq.com>
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-$(CONFIG_TARGET_LOONGSON_2K)		+= platform.o

ifndef CONFIG_SPL_BUILD
obj-y += bootparam.o loongson_efi_systab.o
endif
