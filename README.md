# The Poseidon Server Framework

|Compiler     |Category                   |
|:------------|:--------------------------|
|**GCC 10**   |:1st_place_medal:Primary   |
|**Clang 11** |:2nd_place_medal:Secondary |

![GNU nano for the win!](GNU-nano-FTW.png)

# Features

1. Coroutines (fibers)
2. Native TCP, TLS over TCP and UDP over either IPv4 or IPv6
3. I/O multiplexing basing on epoll
4. HTTP and WebSocket with permessage-deflate
5. Configurable add-ons
6. Asynchronous MySQL, MongoDB and Redis access (optional)

# How to build

#### Prerequisite

```sh
# For Debian, Ubuntu, Linux Mint:
# There is usually an outdated version of meson in the system APT source. Do
# not use it; instead, install the latest one from pip.
sudo apt-get install ninja-build python3 python3-pip pkgconf g++  \
        libpcre2-dev libssl-dev zlib1g-dev libedit-dev
sudo pip3 install meson
```

#### Build and install Asteria

```sh
git submodule update --init
cd asteria/
git checkout origin/master -B master
./makedeb.sh
sudo dpkg -i asteria_*.deb
cd ..
```

#### Build and install http-parser

```sh
git submodule update --init
cd http-parser/
git checkout origin/master -B master
./makedeb.sh
sudo dpkg -i http-parser_*.deb
cd ..
```

#### Build Poseidon

```sh
meson setup build_debug
meson compile -Cbuild_debug
```

#### Start Poseidon in _build_debug_

```sh
LD_LIBRARY_PATH=$(realpath -e build_debug) ./build_debug/poseidon ./etc/poseidon
```

# License

BSD 3-Clause License

# IRC channel:

<ircs://irc.libera.chat:6697/#mcfproj>
