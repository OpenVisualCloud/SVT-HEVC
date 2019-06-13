#!/bin/bash

# Copyright(c) 2018 Intel Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

function clean {
	rm -R -f debug
	rm -R -f release
    rm -R -f ../../Bin/Debug
    rm -R -f ../../Bin/Release
}

function debug {
	mkdir -p debug
	mkdir -p ../../Bin/Debug
	cd debug
	PATH=$PATH:/usr/local/bin/
	cmake ../../.. 					\
		-DCMAKE_BUILD_TYPE=Debug			\
		-DCMAKE_C_COMPILER=$CMAKE_COMPILER		\
		-DCMAKE_ASM_NASM_COMPILER=$CMAKE_ASSEMBLER	\
		-DCMAKE_AR=`which $AR_COMPILER`			\
		-DCMAKE_RANLIB=`which $RANLIB_COMPILER`	\

	# Compile the App
	make -j SvtHevcEncApp
	cd ..
}

function release {
	mkdir -p release
	mkdir -p ../../Bin/Release
	cd release
	PATH=$PATH:/usr/local/bin/
	cmake ../../.. 					\
		-DCMAKE_BUILD_TYPE=Release			\
		-DCMAKE_C_COMPILER=$CMAKE_COMPILER		\
		-DCMAKE_ASM_NASM_COMPILER=$CMAKE_ASSEMBLER	\
		-DCMAKE_AR=`which $AR_COMPILER`			\
		-DCMAKE_RANLIB=`which $RANLIB_COMPILER`	\

	# Compile the App
	make -j SvtHevcEncApp
	cd ..
}

# Defines
CMAKE_ASSEMBLER=yasm
GCC_COMPILER=gcc
AR_COMPILER=gcc-ar
RANLIB_COMPILER=gcc-ranlib
CMAKE_COMPILER=$GCC_COMPILER

if [ $# -eq 0 ]; then
	release
elif [ "$1" = "clean" ]; then
	clean
elif [ "$1" = "debug" ]; then
	debug
elif [ "$1" = "release" ]; then
	release

elif [ "$1" = "all" ]; then
	release
elif [ "$1" = "gcc" ]; then
	release
else
	echo "build.sh <clean|all|debug|release|help>"
fi

exit
