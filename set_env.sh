#!/bin/bash

#set -x
abi2_toolchain_list=("/opt/abcross/loongarch64")

real_target=0
real_toolchain=""

function setup_loongarch_env()
{
	echo "====>setup env for LoongArch..."
	CC_PREFIX=$real_toolchain
	export PATH=$PATH:$CC_PREFIX/bin
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CC_PREFIX/lib
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CC_PREFIX/loongarch64-aosc-linux-gnu/lib

	export ARCH=loongarch
	export CROSS_COMPILE=loongarch64-aosc-linux-gnu-
}

for i in ${abi2_toolchain_list[*]}
do
	real_toolchain=$i
	if [ -d $real_toolchain ]; then
		real_target=1
		break;
	fi
done

if [ $real_target -eq 0 ]; then
	echo "没有找到相关工具链的文件夹"
fi
echo "当前声明的工具链为:"
echo $real_toolchain

setup_loongarch_env

#set +x
