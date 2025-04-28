#!/bin/bash -e

if ! test -d build_release
then
  rm -rf build_release
  meson setup -Ddebug=true -Doptimization=3 build_release
fi

meson compile -Cbuild_release

export LD_LIBRARY_PATH=$(realpath -e build_release)
./build_release/poseidon ./etc/poseidon
