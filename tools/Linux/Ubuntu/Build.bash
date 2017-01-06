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

build_debug=false
build_type="Release"
build_tests="OFF"

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
            build_debug=true
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
        cmake \
        autoconf libtool libtool-bin

    print_subtitle "Source control"
    sudo apt-get -y --quiet install \
        git subversion

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
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON

        make -j $num_cpu -S
        make install

        mark_built
    fi

    #### kNet

    start_target kNet

    if ! is_cloned ; then
        git clone https://github.com/juj/kNet.git kNet
        cd kNet
        git checkout master
    fi

    if ! is_built ; then
        mkdir -p build
        cd build

        cmake .. \
            -DCMAKE_BUILD_TYPE=$build_type \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
            -DUSE_BOOST:BOOL=FALSE

        make -j $num_cpu -S

        mark_built
    fi

    #### Urho3D

    start_target urho3d

    if ! is_cloned ; then
        git clone https://github.com/urho3d/Urho3D.git urho3d
        cd urho3d
        git checkout 5acb708
    fi

    if ! is_built ; then
        # note This must be 'Build' with capital letter
        mkdir -p Build
        cd Build

        cmake ..\
            -DCMAKE_INSTALL_PREFIX=$DEPS \
            -DCMAKE_BUILD_TYPE=$build_type \
            -DURHO3D_LIB_TYPE=SHARED \
            -DURHO3D_ANGELSCRIPT=0 \
            -DURHO3D_LUA=0 \
            -DURHO3D_NETWORK=0 \
            -DURHO3D_PHYSICS=0 \
            -DURHO3D_NAVIGATION=0 \
            -DURHO3D_TOOLS=0 \
            -DURHO3D_SAMPLES=0 \
            -DURHO3D_URHO2D=0

        make -j $num_cpu -S

        cp -L lib/libUrho3D.so $TUNDRA_BIN/

        mark_built
    fi

    #### zlib
    # todo build manually if needed for android etc.

    start_target zlib

    if ! is_cloned ; then
        git clone https://github.com/madler/zlib.git zlib
        cd zlib
        git checkout v1.2.8
    fi

    if ! is_built ; then

        CFLAGS=-fPIC ./configure --prefix=$PWD/build --static

        make -j $num_cpu -S
        make install

        mark_built
    fi

    #### zziplib
    # todo build manually if needed for android etc.

    start_target zziplib

    if ! is_cloned ; then
        wget -O zziplib.tar.bz2 http://sourceforge.net/projects/zziplib/files/zziplib13/0.13.62/zziplib-0.13.62.tar.bz2/download
        tar jxf zziplib.tar.bz2
        mv zziplib-0.13.62 zziplib
        rm -f zziplib.tar.bz2
    fi

    if ! is_built ; then

        ./configure \
            --prefix=$PWD/build \
            --with-zlib=$DEPS_SRC/zlib/build \
            --enable-static \
            --disable-shared \
            --with-pic

        make -j $num_cpu -S
        make install

        mark_built
    fi

    #### polarssl
    # alternative for openssl for curl that should allow static linking
    # but currently libpolarssl.a(ctr_drbg.c.o) has problems even with -fPIC

    start_target polarssl

    if ! is_cloned ; then
        git clone https://github.com/polarssl/polarssl.git polarssl
        cd polarssl
        git checkout polarssl-1.3.9
    fi

    if ! is_built ; then

        cmake . \
            -DCMAKE_INSTALL_PREFIX=$PWD/build \
            -DCMAKE_BUILD_TYPE=$build_type \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
            -DENABLE_PROGRAMS=OFF \
            -DENABLE_TESTING=OFF \
            -DENABLE_ZLIB_SUPPORT=OFF \
            -DUSE_STATIC_POLARSSL_LIBRARY=ON

        make -j $num_cpu -S
        make install

        mark_built
    fi

    #### curl

    start_target curl

    if ! is_cloned ; then
        git clone --branch curl-7_38_0 https://github.com/bagder/curl.git curl
        cd curl
        git checkout curl-7_38_0
    fi

    if ! is_built ; then
        ./buildconf

        ./configure --prefix=$PWD/build \
                    --enable-optimize \
                    --disable-shared --enable-static \
                    --without-ssl --with-polarssl=$DEPS_SRC/polarssl/build \
                    --with-zlib=$DEPS_SRC/zlib/build

        make -j $num_cpu -S
        make install

        cp $DEPS_SRC/polarssl/build/lib/*.a $DEPS_SRC/curl/build/lib/

        mark_built
    fi

    #### Bullet

    start_target bullet

    if ! is_cloned ; then
        git clone https://github.com/bulletphysics/bullet3 bullet
        cd bullet
        git checkout 2.83.6
    fi

    if ! is_built ; then
        mkdir -p build
        cd build

	cmake .. \
            -DCMAKE_INSTALL_PREFIX=$DEPS \
            -DCMAKE_BUILD_TYPE=$build_type \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
            -DBUILD_EXTRAS:BOOL=OFF \
            -DBUILD_UNIT_TESTS:BOOL=OFF \
            -DBUILD_BULLET3:BOOL=OFF \
            -DBUILD_BULLET2_DEMOS:BOOL=OFF \
            -DCMAKE_MINSIZEREL_POSTFIX= -DCMAKE_RELWITHDEBINFO_POSTFIX=

        make -j $num_cpu -S
        make install

        mark_built
    fi

    #### boost

    start_target boost
    if ! is_cloned ; then
        wget http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_1_54_0.tar.gz
        tar -xf boost_1_54_0.tar.gz
        mv boost_1_54_0 boost
        rm boost_1_54_0.tar.gz
    fi
    if ! is_built ; then
        ./bootstrap.sh --prefix=$DEPS --with-libraries=system
        ./b2 cxxflags="-fPIC" install
        mark_built
    fi

    #### websocketpp

    start_target websocketpp

    if ! is_cloned ; then
         git clone https://github.com/realXtend/websocketpp.git websocketpp
         # Simply copy headers to deps include
         cp -r websocketpp/websocketpp ../include
    fi
fi

# Build gtest if testing

if [ $run_tests = true ] ; then

    start_target gtest

    if ! is_cloned ; then
        git clone https://github.com/google/googletest.git gtest
        cd gtest
        git checkout release-1.7.0
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
        -DENABLE_TESTS=$build_tests \
        -DMATHGEOLIB_HOME=$DEPS \
        -DURHO3D_HOME=$DEPS_SRC/urho3d/Build \
        -DKNET_HOME=$DEPS_SRC/kNet \
        -DBULLET_HOME=$DEPS \
        -DGTEST_HOME=$DEPS_SRC/gtest \
        -DCURL_HOME=$DEPS_SRC/curl/build \
        -DZZIPLIB_HOME=$DEPS_SRC/zziplib/build \
        -DBOOST_ROOT=$DEPS
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
