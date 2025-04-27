#!/bin/bash -e

meson compile -Cbuild_debug

export LD_LIBRARY_PATH=$(realpath -e build_debug)
valgrind --leak-check=full -- ./build_debug/poseidon ./etc/poseidon
