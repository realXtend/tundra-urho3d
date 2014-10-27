#!/bin/bash

set -e

# Imports

PARENT_DIR=$(dirname $(readlink -f $0))
source ${PARENT_DIR}/Paths.bash
source ${PARENT_DIR}/Utils.bash

# Functions

print_help()
{
cat << EOF
Usage: $(basename $0) OPTIONS

Options:
  -np, --no-packages     Skip running OS package manager
  -nc, --no-cmake        Skip running Tundra CMake
  -nb, --no-build        Skip building Tundra

  -ra, --run-analysis    Run static analysis with cppcheck
  -rt, --run-tests       Run unit tests

  -d, --debug            Build deps in debug mode.
                           Note: You have to manually destroy
                           the deps dir when changing modes.

  -h, --help             Print this help

  Note: Params cannot be combined to a single command eg. -dnc).

EOF
}

# Misc

export num_cpu=`grep -c "^processor" /proc/cpuinfo`

# Config

skip_pkg=false
skip_deps=false
skip_cmake=false
skip_build=false

run_analysis=false
run_tests=false

build_type="Release"
build_tests="OFF"
build_64bit=false
build_64bit_int=0

# Parse command line args

while [[ $1 = -* ]]; do
    arg=$1; 
    shift

    case $arg in
        --no-packages|-np)
            skip_pkg=true
            ;;
        --no-deps|-nd)
            skip_deps=true
            ;;
        --no-cmake|-nc)
            skip_cmake=true
            ;;
        --no-build|-nb)
            skip_build=true
            ;;
        --run-analysis|-ra)
            run_analysis=true
            ;;
        --run-tests|-rt)
            run_tests=true
            ;;
        --debug|-d)
            build_type="Debug"
            ;;
        --help|-h)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown parameter '$arg'. Run --help for available commands."
            exit 1
            ;;
    esac
done

if [ $(dpkg-architecture -qDEB_HOST_ARCH) = "amd64" ] ; then
    build_64bit=true
    build_64bit_int=1
fi
if [ $run_tests = true ] ; then
    build_tests="ON"
fi

# Init

mkdir -p $TUNDRA_BUILD_DIR
mkdir -p $DEPS_SRC $DEPS_BIN $DEPS_LIB $DEPS_INC

# Packages

if [ $skip_pkg = false ] ; then

    print_title "Fetching packages"

    print_subtitle "Build tools and utils"
    sudo apt-get -y --quiet install \
        build-essential gcc \
        cmake

    print_subtitle "Source control"
    sudo apt-get -y --quiet install \
        git

    print_subtitle "Urho3D"
    sudo apt-get -y --quiet install \
        libx11-dev libxrandr-dev libasound2-dev \
        libgl1-mesa-dev
fi

if [ $skip_deps = false ] ; then

    print_title "Building dependencies"

    #### MathGeoLib

    start_target MathGeoLib

    if ! is_cloned ; then
        git clone https://github.com/juj/MathGeoLib.git MathGeoLib
    fi

    if ! is_built ; then
        mkdir -p build
        cd build

        cmake .. \
            -DCMAKE_INSTALL_PREFIX=$DEPS \
            -DCMAKE_BUILD_TYPE=$build_type \
            -DCMAKE_CXX_FLAGS="-fPIC"

        make -j $num_cpu -S
        make install

        mark_built
    fi

    #### kNet

    start_target kNet

    if ! is_cloned ; then
        git clone https://github.com/juj/kNet.git kNet
    fi

    if ! is_built ; then
        mkdir -p build
        cd build

        cmake .. \
            -DCMAKE_BUILD_TYPE=$build_type \
            -DCMAKE_CXX_FLAGS="-fPIC" \
            -DUSE_BOOST:BOOL=FALSE

        make -j $num_cpu -S

        mark_built
    fi

    #### Urho3D

    start_target urho3d

    if ! is_cloned ; then
        git clone https://github.com/urho3d/Urho3D.git urho3d
    fi

    if ! is_built ; then
        # note This must be 'Build' with capital letter
        mkdir -p Build
        cd Build

        cmake ../Source \
            -DCMAKE_INSTALL_PREFIX=$DEPS \
            -DCMAKE_BUILD_TYPE=$build_type \
            -DURHO3D_64BIT=$build_64bit_int \
            -DURHO3D_LIB_TYPE=SHARED \
            -DURHO3D_ANGELSCRIPT=0 \
            -DURHO3D_LUA=0 \
            -DURHO3D_NETWORK=0 \
            -DURHO3D_PHYSICS=0 \
            -DURHO3D_TOOLS=0

        make -j $num_cpu -S
        
        cp -L ../Lib/libUrho3D.so $TUNDRA_BIN/

        mark_built
    fi
fi

# Build gtest if testing

if [ $run_tests = true ] ; then

    start_target gtest

    if ! is_cloned ; then
        svn checkout http://googletest.googlecode.com/svn/tags/release-1.7.0/ gtest
    fi

    if ! is_built ; then
        mkdir -p build
        cd build

        cmake ../ \
            -DCMAKE_BUILD_TYPE=$build_type

        make -j $num_cpu -S

        mark_built
    fi
fi

# Build cppcheck if analysing

if [ $run_analysis = true ] ; then

    start_target cppcheck

    if ! is_cloned ; then
        wget -nv -O cppcheck-1.66.tar.gz http://sourceforge.net/projects/cppcheck/files/cppcheck/1.66/cppcheck-1.66.tar.gz/download
        tar -zxf cppcheck-1.66.tar.gz

        mv cppcheck-1.66 cppcheck
        rm cppcheck-1.66.tar.gz
    fi

    if ! is_built ; then
        make -j $num_cpu -S cppcheck

        mark_built
    fi
fi

# Tundra cmake

if [ $skip_cmake = false ] ; then

    print_title "Running Tundra CMake"

    cd $TUNDRA_BUILD_DIR

    if file_exists CMakeCache.txt ; then
        rm CMakeCache.txt
    fi

    cmake .. \
        -DCMAKE_CXX_FLAGS="-Wno-unused-parameter -Wno-unused-variable" \
        -DCMAKE_BUILD_TYPE=$build_type \
        -DMATHGEOLIB_HOME=$DEPS \
        -DURHO3D_HOME=$DEPS_SRC/urho3d \
        -DKNET_HOME=$DEPS_SRC/kNet \
        -DGTEST_HOME=$DEPS_SRC/gtest \
        -DENABLE_TESTS=$build_tests
fi

# Tundra build

if [ $skip_build = false ] ; then

    print_title "Building Tundra"

    cd $TUNDRA_BUILD_DIR

    make -j $num_cpu -S
fi

# Run static code analysis

if [ $run_analysis = true ] ; then

    print_title "Running cppcheck"

    $DEPS_SRC/cppcheck/cppcheck \
        --template "{file}({line}): ({severity}) ({id}): {message}" \
        --enable=all \
        --suppress=missingInclude \
        --suppress=missingIncludeSystem \
        --suppress=unusedFunction \
        --suppressions-list=$TUNDRA/tools/Windows/Cppcheck/Suppressions.txt \
        --std=c++11 \
        --error-exitcode=0 \
        --relative-paths=$TUNDRA_SRC \
        -DTUNDRACORE_API= \
        -I $TUNDRA_SRC \
        $TUNDRA_SRC
fi

# Unit tests

if [ $run_tests = true ] ; then
    print_title "Running tests"

    cd $TUNDRA_BUILD_DIR
    make RUN_ALL_TESTS
fi
