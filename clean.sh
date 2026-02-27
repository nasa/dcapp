#!/bin/bash

# find directory of this script
SOURCE=${BASH_SOURCE[0]}
while [ -h "$SOURCE" ]; do
  DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
  SOURCE=$(readlink "$SOURCE")
  [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
done
DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

pushd $DIR >/dev/null

# app artifacts
rm -rf pilotlight/out
rm -rf pilotlight/out-temp
rm -rf pilotlight/shader-temp
rm -rf pilotlight/cache

# sample artifacts
rm -f samples/*/logic/logic.so
rm -f samples/*/logic/logic_*.so

popd >/dev/null
