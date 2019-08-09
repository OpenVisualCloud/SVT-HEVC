#!/bin/bash

# Copyright(c) 2018 Intel Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

# Fails the script if any of the commands error (Other than if and some others)
set -e

# Allow use for coloring (as long as it's within the lines)
if [ -n "$(tput colors 2> /dev/null)" ] && [ "$(tput colors 2> /dev/null)" -ge 8 ]; then
    bold=$(tput bold)
    blue=$(tput setaf 12)
    yellow=$(tput setaf 11)
    purple=$(tput setaf 13)
    green=$(tput setaf 2)
    red=$(tput setaf 1)
    reset=$(tput sgr0)
fi

# Function to print to stderr or stdout while stripping preceding tabs(spaces)
# If tabs are desired, use cat
print_message() {
    if [ "$1" = "stdout" ] || [ "$1" = "-" ]; then
        shift
        printf "%b\\n" "${@:-Unknown Message}" | sed -e 's/^[ \t]*//'
    else
        printf "%b\\n" "${@:-Unknown Message}" | sed -e 's/^[ \t]*//' >&2
    fi
}

# Convenient function to exit with message
die() {
    if [ -z "$*" ]; then
        print_message "${red}Unknown error${reset}"
    else
        print_message "$@"
    fi
    print_message "The script will now exit."
    exit 1
}

# Function for changing directory.
cd_safe() {
    if ! cd "$1"; then
        shift
        if [ -n "$*" ]; then
            die "$@"
        else
            die "Failed cd to $1."
        fi
    fi
}

# info about self
script_dir="$(cd "$(dirname "$0")" > /dev/null 2>&1 && pwd)"
cd_safe "$script_dir"
THIS_SCRIPT=$0

# Help message
echo_help() {
    cat << EOF
Usage: $THIS_SCRIPT [OPTION] ... -- [OPTIONS FOR CMAKE]
-a, --all, all          Builds release and debug
    --asm, asm=*        Set assembly compiler [$ASM]
-b, --bindir, bindir=*  Directory to install binaries
    --cc, cc=*          Set C compiler [$CC]
    --cxx, cxx=*        Set CXX compiler [$CXX]
    --clean, clean      Remove build and Bin folders
    --debug, debug      Build debug
    --shared, shared    Build shared libs
-x, --static, static    Build static libs
-g, --gen, gen=*        Set CMake generator
-i, --install, install  Install build [Default release]
-j, --jobs, jobs=*      Set number of jobs for make/CMake [$jobs]
-p, --prefix, prefix=*  Set installation prefix
    --release, release  Build release
-s, --target_system,    Set CMake target system
    target_system=*
    --test, test        Build Unit Tests
-t, --toolchain,        Set CMake toolchain file
    toolchain=*
-v, --verbose, verbose  Print out commands
Example usage:
    build.sh -xi debug test
    build.sh jobs=8 all cc=clang cpp=clang++
    build.sh -j 4 all -t "https://gist.githubusercontent.com/peterspackman/8cf73f7f12ba270aa8192d6911972fe8/raw/mingw-w64-x86_64.cmake"
    build.sh generator=Xcode cc=clang
EOF
}

# Usage: build <release|debug> [test]
build() (
    local build_type match
    while [ -n "$*" ]; do
        match=$(echo "$1" | tr '[:upper:]' '[:lower:]')
        case "$match" in
        release) build_type="release" && shift ;;
        debug) build_type="debug" && shift ;;
        *) break ;;
        esac
    done

    mkdir -p ${build_type:-release} > /dev/null 2>&1
    cd_safe ${build_type:-release}
    [ -f CMakeCache.txt ] && rm CMakeCache.txt
    [ -d CMakeFiles ] && rm -rf CMakeFiles
    cmake ../../.. -DCMAKE_BUILD_TYPE="${build_type:-release}" "${CMAKE_EXTRA_FLAGS[@]}" "$@"

    # Compile the Library
    if [ -f Makefile ]; then
        make -j "$jobs"
    else
        cmake --build . --config "${build_type:-release}"
    fi
    cd_safe ..
)

check_executable() {
    while true; do
        case "$1" in
        -p)
            local print_exec
            print_exec=y
            shift
            ;;
        *) break ;;
        esac
    done
    local command_to_check
    command_to_check="$1"
    if command -v "$command_to_check" > /dev/null 2>&1; then
        [ -n "$print_exec" ] && command -v "$command_to_check"
        return 0
    else
        if [ $# -gt 1 ]; then
            shift
            local hints
            hints=("$@")
            for d in "${hints[@]}"; do
                if [ -e "$d/$command_to_check" ]; then
                    [ -n "$print_exec" ] && echo "$d/$command_to_check"
                    return 0
                fi
            done
        fi
        return 127
    fi
}

install_build() {
    local build_type
    build_type="$(echo "$1" | tr '[:upper:]' '[:lower:]')"
    check_executable sudo && sudo -v > /dev/null 2>&1 && local sudo=sudo
    [ -d "${build_type:-release}" ] &&
        cd_safe "${build_type:-release}" "Unable to find the build folder. Did the build command run?"
    $sudo cmake --build . --target install \
        --config "${build_type:-release}" ||
        die "Unable to run install"
    cd_safe ..
}

parse_options() {
    while [ -n "$*" ]; do
        case $1 in
        help)
            echo_help
            exit
            ;;
        asm=*)
            check_executable "${1#*=}" &&
                CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DCMAKE_ASM_NASM_COMPILER=$(check_executable -p "${1#*=}")")
            shift
            ;;
        bindir=*)
            CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-CMAKE_INSTALL_BINDIR=${1#*=}")
            shift
            ;;
        cc=*)
            if check_executable "${1#*=}"; then
                export CC
                CC="$(check_executable -p "${1#*=}")"
            fi
            shift
            ;;
        cxx=*)
            if check_executable "${1#*=}"; then
                export CXX
                CXX="$(check_executable -p "${1#*=}")"
            fi
            shift
            ;;
        clean)
            for d in *; do
                [ -d "$d" ] && rm -rf "$d"
            done
            for d in ../../Bin/*; do
                [ -d "$d" ] && rm -rf "$d"
            done
            exit
            ;;
        debug) build_debug=y && shift ;;
        enable_shared) CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DBUILD_SHARED_LIBS=ON") && shift ;; #
        disable_shared) CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DBUILD_SHARED_LIBS=OFF") && shift ;;
        generator=*) CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" -G"${1#*=}") && shift ;;
        install) build_install=y && shift ;;
        jobs=*) jobs="${1#*=}" && shift ;; #
        prefix=*) CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DCMAKE_INSTALL_PREFIX=${1#*=}") && shift ;;
        release) build_release=y && shift ;;
        target_system=*)
            CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DCMAKE_SYSTEM_NAME=${1#*=}")
            shift
            ;;
        tests) CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DBUILD_TESTING=ON") && shift ;;
        toolchain=*)
            local toolchain url
            url="${1#*=}"
            if [ "${url:0:4}" = "http" ]; then
                toolchain="${url%%\?*}"
                toolchain="${toolchain##*/}"
                curl --connect-timeout 15 --retry 3 --retry-delay 5 -sfLk -o "$toolchain" "$url"
                toolchain="$script_dir/$toolchain"
            else
                toolchain="$url"
            fi
            CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DCMAKE_TOOLCHAIN_FILE=../$toolchain") && shift
            ;;
        verbose) CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DCMAKE_VERBOSE_MAKEFILE=1") && shift ;;
        esac
    done
}

# Defines
uname=$(uname -a)
if [ -z "$CC" ] && [ "${uname:0:5}" != "MINGW" ]; then
    export CC
    if check_executable icc "/opt/intel/bin"; then
        CC=$(check_executable -p icc "/opt/intel/bin")
    elif check_executable gcc; then
        CC=$(check_executable -p gcc)
    elif check_executable clang; then
        CC=$(check_executable -p clang)
    else
        CC=$(check_executable -p cc)
    fi
fi

if [ -z "$CXX" ] && [ "${uname:0:5}" != "MINGW" ]; then
    export CXX
    if check_executable icpc "/opt/intel/bin"; then
        CXX=$(check_executable -p icpc "/opt/intel/bin")
    elif check_executable g++; then
        CXX=$(check_executable -p g++)
    elif check_executable clang++; then
        CXX=$(check_executable -p clang++)
    else
        CXX=$(check_executable -p c++)
    fi
fi

if [ -z "$ASM" ] && [ "${uname:0:5}" != "MINGW" ]; then
    if check_executable yasm; then
        ASM=$(check_executable -p yasm)
    elif check_executable nasm; then
        ASM=$(check_executable -p nasm)
    else
        die "Suitable asm compiler not found."
    fi
    if [ -n "$ASM" ]; then
        CMAKE_EXTRA_FLAGS=("${CMAKE_EXTRA_FLAGS[@]}" "-DCMAKE_ASM_NASM_COMPILER=$ASM")
    fi
fi

if [ -z "$jobs" ]; then
    if getconf _NPROCESSORS_ONLN > /dev/null 2>&1; then
        jobs=$(getconf _NPROCESSORS_ONLN)
    elif check_executable nproc; then
        jobs=$(nproc)
    elif check_executable sysctl; then
        jobs=$(sysctl -n hw.ncpu)
    else
        jobs=2
    fi
fi

if [ -z "$*" ]; then
    build_release=y
else
    while [ -n "$*" ]; do
        match=""
        if [ "${1:0:2}" = "--" ]; then
            [ -z "${1:2}" ] && shift && break
            match=$(echo "${1:2}" | tr '[:upper:]' '[:lower:]')
            case "$match" in
            help)
                parse_options help
                shift
                ;;
            all)
                parse_options debug release
                shift
                ;;
            asm)
                parse_options asm="$2"
                shift 2
                ;;
            bindir)
                parse_options bindir="$2"
                shift 2
                ;;
            cc)
                parse_options cc="$2"
                shift 2
                ;;
            cxx)
                parse_options cxx="$2"
                shift 2
                ;;
            clean)
                parse_options clean
                shift
                ;;
            debug)
                parse_options debug
                shift
                ;;
            gen)
                parse_options generator="$2"
                shift 2
                ;;
            install)
                parse_options install
                shift
                ;;
            jobs)
                parse_options jobs="$2"
                shift 2
                ;;
            prefix)
                parse_options prefix="$2"
                shift 2
                ;;
            release)
                parse_options release
                shift
                ;;
            shared)
                parse_options enable_shared
                shift
                ;;
            no-shared)
                parse_options disable_shared
                shift
                ;;
            static)
                parse_options disable_shared
                shift
                ;;
            target_system)
                parse_options target_system="$2"
                shift 2
                ;;
            toolchain)
                parse_options toolchain="$2"
                shift
                ;;
            test)
                parse_options tests
                shift
                ;;
            verbose)
                parse_options verbose
                shift
                ;;
            *) die "Error, unknown option: $1" ;;
            esac
        elif [ "${1:0:1}" = "-" ]; then
            i=1
            opt=""
            while [ $i -lt ${#1} ]; do
                opt=$(echo "${1:$i:1}" | tr '[:upper:]' '[:lower:]')
                case "$opt" in
                h) parse_options help ;;
                a) parse_options all && i=$((i+1)) ;;
                b) parse_options bindir="$1" && i=$((i+1)) ;;
                g) parse_options generator="$1" && i=$((i+1)) ;;
                i) parse_options install && i=$((i+1)) ;;
                j) parse_options jobs="$1" && i=$((i+1)) ;;
                p) parse_options prefix="$1" && i=$((i+1)) ;;
                s) parse_options target_system="$1" && i=$((i+1)) ;;
                t) parse_options toolchain="$1" && i=$((i+1)) ;;
                x) parse_options disable_shared && i=$((i+1)) ;;
                v) parse_options verbose && i=$((i+1)) ;;
                *) die "Error, unknown option: -$opt" ;;
                esac
                i=$((i + 1))
            done
            shift
        else
            match=$(echo "$1" | tr '[:upper:]' '[:lower:]')
            case "$match" in
            all)
                parse_options release debug
                shift
                ;;
            asm=*)
                parse_options asm="${1#*=}"
                shift
                ;;
            bindir=*)
                parse_options bindir="${1#*=}"
                shift
                ;;
            cc=*)
                parse_options cc="${1#*=}"
                shift
                ;;
            cxx=*)
                parse_options cxx="${1#*=}"
                shift
                ;;
            clean)
                parse_options clean
                shift
                ;;
            debug)
                parse_options debug
                shift
                ;;
            gen=*)
                parse_options generator="${1#*=}"
                shift
                ;;
            help)
                parse_options help
                shift
                ;;
            install)
                parse_options install
                shift
                ;;
            jobs=*)
                parse_options jobs="${1#*=}"
                shift
                ;;
            prefix=*)
                parse_options prefix="${1#*=}"
                shift
                ;;
            target_system=*)
                parse_options target_system="${1#*=}"
                shift
                ;;
            shared)
                parse_options enable_shared
                shift
                ;;
            no-shared)
                parse_options disable_shared
                shift
                ;;
            static)
                parse_options disable_shared
                shift
                ;;
            release)
                parse_options release
                shift
                ;;
            test)
                parse_options tests
                shift
                ;;
            toolchain=*)
                parse_options toolchain="${1#*=}"
                shift
                ;;
            verbose)
                parse_options verbose
                shift
                ;;
            end) exit ;;
            *) die "Error, unknown option: $1" ;;
            esac
        fi
    done
fi

[[ ":$PATH:" != *":/usr/local/bin:"* ]] && PATH="$PATH:/usr/local/bin"

if [ "$build_debug" = "y" ] && [ "$build_release" = "y" ]; then
    build release "$@"
    build debug "$@"
elif [ "$build_debug" = "y" ]; then
    build debug "$@"
else
    build release "$@"
fi

if [ -n "$build_install" ]; then
    if [ -n "$build_release" ]; then
        install_build release
    elif [ -n "$build_debug" ]; then
        install_build debug
    else
        build release "$@"
        install_build release
    fi
fi
