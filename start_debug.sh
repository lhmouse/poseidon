#!/bin/bash -e

meson compile -Cbuild_debug

export LD_LIBRARY_PATH=$(realpath -e build_debug)
export LD_PRELOAD=$(ldd ./build_debug/libposeidon.so |
                    grep -Eho '\<lib(a|ub|t)san\.so\>' |
                    sort -u)
./build_debug/poseidon ./etc/poseidon
