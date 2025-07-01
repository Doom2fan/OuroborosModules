#!/usr/bin/env bash

RACK_DIR=${RACK_DIR:="../.."}

BUILD_TYPE="Release"

if ! options=$(getopt -o rd -l release,debug -- "$@")
then
    # Error, getopt will put out a message for us
    exit 1
fi

set -- $options

while [ $# -gt 0 ]
do
    # Consume next (1st) argument
    case $1 in
    -r|--release)
      BUILD_TYPE="Release" ;;
    -d|--debug)
      BUILD_TYPE="Debug" ;;
    (--)
      shift; break;;
    (-*)
      echo "$0: error - unrecognized option $1" 1>&2; exit 1;;
    (*)
      break;;
    esac
    # Fetch next argument as 1st
    shift
done

CMAKE_BUILD=dep/cmake-build_$BUILD_TYPE
cmake -B $CMAKE_BUILD -DRACK_SDK_DIR=$RACK_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX=$CMAKE_BUILD/dist
cmake --build $CMAKE_BUILD -- -j $(getconf _NPROCESSORS_ONLN)
cmake --install $CMAKE_BUILD