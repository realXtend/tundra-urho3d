#!/bin/bash

set -e

PARENT_DIR=$(dirname $(readlink -f $0))

export TUNDRA=$(cd $PARENT_DIR/../../.. && pwd)
export TUNDRA_BIN=$TUNDRA/bin

export DEPS_DIR_NAME="deps-$(uname -n)-$(lsb_release -sr)-$(dpkg-architecture -qDEB_HOST_ARCH)"
export DEPS=$TUNDRA/$DEPS_DIR_NAME
export DEPS_SRC=$DEPS/src
export DEPS_BIN=$DEPS/bin
export DEPS_LIB=$DEPS/lib
export DEPS_INC=$DEPS/include
