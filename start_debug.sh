#!/bin/bash -e

if ! test -d build_debug
then
  rm -rf build_debug
  meson setup -Ddebug=true -Doptimization=0 -Denable-debug-checks=true build_debug
fi

meson compile -Cbuild_debug

export LD_LIBRARY_PATH=$(realpath -e build_debug)
valgrind -- ./build_debug/poseidon ./etc/poseidon
