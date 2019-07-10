:: Copyright(c) 2018 Intel Corporation
:: SPDX-License-Identifier: BSD-2-Clause-Patent
@echo off

setlocal
cd /d "%~dp0"
set instdir=%CD%

set "build=y"
set "Release=y"
set "Debug=y"
set "cmake_eflags=-DCMAKE_INSTALL_PREFIX=^"%SYSTEMDRIVE%\svt-encoders^""
if NOT -%1-==-- call :args %*
if exist CMakeCache.txt del /f /s /q CMakeCache.txt 1>nul
if exist CMakeFiles rmdir /s /q CMakeFiles 1>nul

setlocal ENABLEDELAYEDEXPANSION
for /f "usebackq tokens=2" %%f in (`cmake -G 2^>^&1 ^| findstr /i ^*`) do (
    if "%Release%"=="y" (
        if "%Debug%"=="y" (
            if "%GENERATOR:~0,6%"=="Visual" (
                set "buildtype=Release;Debug"
            ) else if NOT "%GENERATOR%"=="" (
                set "buildtype=Debug"
            ) else if "%%f"=="Visual" (
                set "buildtype=Release;Debug"
            )
        ) else (
            set "buildtype=Release"
        )
    ) else set "buildtype=Debug"

    if "%GENERATOR:~0,6%"=="Visual" (
        set "cmake_eflags=-DCMAKE_CONFIGURATION_TYPES=^"!buildtype!^" %cmake_eflags%"
    ) else if NOT "%GENERATOR%"=="" (
        set "cmake_eflags=-DCMAKE_BUILD_TYPE=^"!buildtype!^" %cmake_eflags%"
    ) else if "%%f"=="Visual" (
        set "cmake_eflags=-DCMAKE_CONFIGURATION_TYPES=^"!buildtype!^" %cmake_eflags%"
    )
)
endlocal

if "%GENERATOR%"=="Visual Studio 16 2019" (
    cmake ../.. -G"Visual Studio 16 2019" -A x64 %cmake_eflags%
) else if NOT "%GENERATOR%"=="" (
    cmake ../.. -G"%GENERATOR%" %cmake_eflags%
) else (
    cmake ../.. %cmake_eflags%
)

for /f "usebackq tokens=*" %%f in (`cmake --build 2^>^&1 ^| findstr /i jobs`) do (
    if NOT "%%f"=="" set "build_flags=-j %NUMBER_OF_PROCESSORS% %build_flags%"
)
if "%build%"=="y" (
    if "%Release%"=="y" cmake --build . --config Release %build_flags%
    if "%Debug%"=="y" cmake --build . --config Debug %build_flags%
)
goto :EOF

:args
if -%1-==-- (
    exit /b
) else if /I "%1"=="/help" (
    call :help
) else if /I "%1"=="help" (
    call :help
) else if /I "%1"=="clean" (
    echo Cleaning build folder
    for %%i in (*) do if not "%%~i" == "build.bat" del "%%~i"
    for /d %%i in (*) do if not "%%~i" == "build.bat" (
        del /f /s /q "%%~i" 1>nul
        rmdir /s /q "%%~i" 1>nul
    )
    exit
) else if /I "%1"=="2019" (
    echo Building for Visual Studio 2019
    set "GENERATOR=Visual Studio 16 2019"
    shift
) else if /I "%1"=="2017" (
    echo Building for Visual Studio 2017
    set "GENERATOR=Visual Studio 15 2017 Win64"
    shift
) else if /I "%1"=="2015" (
    echo Building for Visual Studio 2015
    set "GENERATOR=Visual Studio 14 2015 Win64"
    shift
) else if /I "%1"=="2013" (
    echo Building for Visual Studio 2013
    echo This is currently not officially supported
    set "GENERATOR=Visual Studio 12 2013 Win64"
    shift
) else if /I "%1"=="2012" (
    echo Building for Visual Studio 2012
    echo This is currently not officially supported
    set "GENERATOR=Visual Studio 11 2012 Win64"
    shift
) else if /I "%1"=="2010" (
    echo Building for Visual Studio 2010
    echo This is currently not officially supported
    set "GENERATOR=Visual Studio 10 2010 Win64"
    shift
) else if /I "%1"=="2008" (
    echo Building for Visual Studio 2008
    echo This is currently not officially supported
    set "GENERATOR=Visual Studio 9 2008 Win64"
    shift
) else if /I "%1"=="ninja" (
    echo Building for Ninja files
    echo This is currently not officially supported
    set "GENERATOR=Ninja"
    shift
) else if /I "%1"=="msys" (
    echo Building for MSYS Makefiles
    echo This is currently not officially supported
    set "GENERATOR=MSYS Makefiles"
    shift
) else if /I "%1"=="mingw" (
    echo Building for MinGW Makefiles
    echo This is currently not officially supported
    set "GENERATOR=MinGW Makefiles"
    shift
) else if /I "%1"=="unix" (
    echo Building for Unix Makefiles
    echo This is currently not officially supported
    set "GENERATOR=Unix Makefiles"
    shift
) else if /I "%1"=="release" (
    echo Building release only
    set "Release=y"
    set "Debug=n"
    shift
) else if /I "%1"=="debug" (
    echo Building for debug only
    set "Release=n"
    set "Debug=y"
    shift
) else if /I "%1"=="all" (
    echo Building release and debug
    set "Release=y"
    set "Debug=y"
    shift
) else if /I "%1"=="nobuild" (
    echo Generating solution only
    set "build=n"
    shift
) else (
    echo Unknown Token "%1"
    exit 1
)
goto :args

:help
    echo Batch file to build SVT-HEVC on Windows
    echo Usage: build.bat [2019^|2017^|2015^|clean] [release|debug] [nobuild]
    exit
goto :EOF
