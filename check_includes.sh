#!/bin/bash -e

export CPPFLAGS="-D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ibuild_debug"
export CXX=${CXX:-"g++"}

find ${PWD}/"poseidon" -name "*.[hc]pp"  \
  | xargs -n 1 -P $(nproc) --  \
    sh -xec '${CXX} ${CPPFLAGS} -x c++ -std=c++17 -mavx $1 -S -o /dev/null' 0
