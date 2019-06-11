:: Copyright(c) 2018 Intel Corporation
:: SPDX-License-Identifier: BSD-2-Clause-Patent
@echo off

setlocal
cd /d "%~dp0"
set instdir=%CD%

set "build=y"
if NOT -%1-==-- call :args %*
if exist CMakeCache.txt del /f /s /q CMakeCache.txt 1>nul
if exist CMakeFiles rmdir /s /q CMakeFiles 1>nul
if "%buildtype%"=="" set "release"
if "%GENERATOR%"=="Visual Studio 16 2019" (
    cmake ../.. -G"Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=%SYSTEMDRIVE%\svt-encoders -DCMAKE_CONFIGURATION_TYPES="%buildtype%" %cmake_eflags%
) else (
    if NOT "%GENERATOR%"=="" set GENERATOR=-G"%GENERATOR%"
    cmake ../.. %GENERATOR% -DCMAKE_INSTALL_PREFIX=%SYSTEMDRIVE%\svt-encoders -DCMAKE_CONFIGURATION_TYPES="%buildtype%" %cmake_eflags%
)

if "%build%"=="y" cmake --build . --config %buildtype%
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
    echo Building for release
    set "buildtype=release"
    shift
) else if /I "%1"=="debug" (
    echo Building for debug
    set "buildtype=debug"
    shift
) else if /I "%1"=="nobuild" (
    echo Building files
    set "build=n"
    shift
)
goto :args

:help
    echo Batch file to build SVT-HEVC on Windows
    echo Usage: generate.bat 2019^|2017^|2015^|clean [release|debug] [nobuild]
    exit
goto :EOF