#!/bin/bash -e

export LD_LIBRARY_PATH=$(realpath -e build_debug)
./build_debug/poseidon ./etc/poseidon
