#!/bin/bash

set -e

PARENT_DIR=$(dirname $(readlink -f $0))

export TUNDRA=$(cd $PARENT_DIR/../../.. && pwd)
export TUNDRA_BIN=$TUNDRA/bin
export TUNDRA_SRC=$TUNDRA/src

export DEPS_DIR_NAME="deps-$(uname -n)-$(lsb_release -sr)-$(dpkg-architecture -qDEB_HOST_ARCH)"
export DEPS=$TUNDRA/$DEPS_DIR_NAME
export DEPS_SRC=$DEPS/src
export DEPS_BIN=$DEPS/bin
export DEPS_LIB=$DEPS/lib
export DEPS_INC=$DEPS/include

export DEPS_WINDOWS_PATCH_DIR=$TUNDRA/tools/Windows/Patches

export TUNDRA_BUILD_DIR_NAME="build-$(uname -n)-$(lsb_release -sr)-$(dpkg-architecture -qDEB_HOST_ARCH)"
export TUNDRA_BUILD_DIR=$TUNDRA/$TUNDRA_BUILD_DIR_NAME
