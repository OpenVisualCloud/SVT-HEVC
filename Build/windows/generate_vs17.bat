:: Copyright(c) 2018 Intel Corporation 
:: SPDX-License-Identifier: BSD-2-Clause-Patent

cmake ../.. "-GVisual Studio 15 2017 Win64" -DCMAKE_ASM_NASM_COMPILER="yasm.exe" -DCMAKE_INSTALL_PREFIX=%SYSTEMDRIVE%\svt-encoders -DCMAKE_CONFIGURATION_TYPES="Debug;Release"
pause
