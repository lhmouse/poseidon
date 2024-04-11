#!/bin/bash -e

meson compile -Cbuild_release

export LD_LIBRARY_PATH=$(realpath -e build_release)
./build_release/poseidon ./etc/poseidon
