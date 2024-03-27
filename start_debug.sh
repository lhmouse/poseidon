#!/bin/bash -e

_san_so_list=$(ldd ./build_debug/libposeidon.so |
               grep -Eho '\<lib(a|ub|t)san\.so.[0-9]+\>' |
               sort -u)

export LD_LIBRARY_PATH=$(realpath -e build_debug)
export LD_PRELOAD=$(echo ${_san_so_list})
./build_debug/poseidon ./etc/poseidon
