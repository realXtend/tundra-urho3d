#!/bin/bash

COROR_GREEN="\e[0;32m"
COROR_GREY="\e[0;90m"
COLOR_BLUE="\e[0;94m"
COROR_END="\033[0m"

print_title()
{
    echo
    echo -e "${COLOR_BLUE}$1${COROR_END}"
    local len=$((${#1}+1))
    echo -e "${COROR_GREY}`seq -s- ${len} | tr -d '[:digit:]'`${COROR_END}"
}

print_subtitle()
{
    echo
    echo -e "${COROR_GREEN}$1${COROR_END}"
}

dir_exists()
{
    test -d "$1"
    return $?
}

file_exists()
{
    test -f "$1"
    return $?
}

rm_r_quiet()
{
    echo "    Removing $1"
    rm -rf $1
}

rm_quiet()
{
    echo "    Removing $1"
    rm -f $1
}

array_contains()
{
  local e
  for e in "${@:2}"; do [[ "$e" == "$1" ]] && return 0; done
  return 1
}

start_target()
{
    _utils_current_dir_name=$1
    _utils_current_dir_path=$DEPS_SRC/$1

    print_subtitle $_utils_current_dir_name
}

is_built()
{
    # Directory exists?
    test -d "$_utils_current_dir_path"
    if [ $? -ne 0 ] ; then
        echo "Error: Sources not found from $_utils_current_dir_path"
        exit 1
    fi

    # Build done marker file exists?
    test -f "$_utils_current_dir_path/tundra-urho3d-build.meta"
    result=$?
    if [ $result -eq 0 ] ; then
        echo "    Build      OK"
        cd $DEPS
    else
        echo "    Building, please wait..."
        cd $_utils_current_dir_path
    fi
    return $result
}

is_cloned()
{
    # Directory exists?
    test -d "$_utils_current_dir_path"
    result=$?
    if [ $result -eq 0 ] ; then
        echo "    Clone      OK"
    else
        echo "    Cloning, please wait..."
        cd $DEPS_SRC
    fi
    return $result
}

mark_built()
{
    touch $_utils_current_dir_path/tundra-urho3d-build.meta
}
