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
  -d, --debug            Build deps in debug mode.

  Note: Params cannot be combined to a single command eg. -ncnb).

EOF
}

# Misc

export num_cpu=`grep -c "^processor" /proc/cpuinfo`

# Config

skip_pkg=false
skip_deps=false
skip_cmake=false
skip_build=false

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

mkdir -p $DEPS_SRC $DEPS_BIN $DEPS_LIB $DEPS_INC

# Packages

if [ $skip_pkg = false ] ; then

    print_title "Fetching packages"

    print_subtitle "Build tools and utils"
    sudo apt-get -y --quiet install \
        build-essential gcc cmake

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
        cmake . \
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
