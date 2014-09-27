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

build_type="Release"
build_64bit=false
build_64bit_int=0

if [ $(dpkg-architecture -qDEB_HOST_ARCH) = "amd64" ] ; then
    build_64bit=true
    build_64bit_int=1
fi


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
            -DCMAKE_BUILD_TYPE=$build_type

        make -j $num_cpu -S
        make install

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
            -DURHO3D_TOOLS=0

        make -j $num_cpu -S
        
        cp -L ../Lib/libUrho3D.so $TUNDRA_BIN/

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
        -DMATHGEOLIB_HOME=$DEPS \
        -DURHO3D_HOME=$DEPS_SRC/urho3d
fi

# Tundra build

if [ $skip_build = false ] ; then

    print_title "Building Tundra"

    cd $TUNDRA_BUILD_DIR

    make -j $num_cpu -S
fi

# Run static code analysis

if [ $run_analysis = true ] ; then

    # Build cppcheck (one in apt-get is ancient on travis ci machines)

    start_target cppcheck

    if ! is_cloned ; then
        wget -O cppcheck-1.66.tar.gz http://sourceforge.net/projects/cppcheck/files/cppcheck/1.66/cppcheck-1.66.tar.gz/download
        tar -zxvf cppcheck-1.66.tar.gz

        mv cppcheck-1.66 cppcheck
        rm cppcheck-1.66.tar.gz
    fi

    if ! is_built ; then
        make -j $num_cpu -S

        mark_built
    fi

    print_title "Running cppcheck"

    $DEPS_SRC/cppcheck/cppcheck \
        --template "{file}({line}): ({severity}) ({id}): {message}" \
        --enable=all \
        --suppress=missingInclude \
        --suppress=missingIncludeSystem \
        --std=c++11 \
        --error-exitcode=1 \
        --relative-paths=$TUNDRA_SRC \
        --check-config \
        -DTUNDRACORE_API= \
        -I $TUNDRA_SRC \
        $TUNDRA_SRC
fi
