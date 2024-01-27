# The Poseidon Server Framework

|Compiler     |Category                   |
|:------------|:--------------------------|
|**GCC 10**   |:1st_place_medal:Primary   |
|**Clang 11** |:2nd_place_medal:Secondary |

![GNU nano for the win!](GNU-nano-FTW.png)

# Features

1. Coroutines (fibers)
2. Native **TCP**, **TLS over TCP** and **UDP** over either **IPv4** or **IPv6**
3. I/O multiplexing basing on **epoll**
4. **HTTP** and **WebSocket** with **permessage-deflate**
5. Configurable add-ons
6. Asynchronous **MySQL** access (optional, WIP)
7. Asynchronous **MongoDB** access (optional, WIP)

# How to build

#### Prerequisite

```sh
sudo aptitude install --no-install-recommends  \
   zlib1g-dev lib{pcre2,ssl,magic,http-parser,unwind}-dev  \
   lib{mysqlclient,mongoc}-dev
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
meson setup build_dir
cd build_dir
ninja
```

#### Start Poseidon in _build_dir_

```sh
./poseidon ../etc/poseidon
```

# License

BSD 3-Clause License

# IRC channel:

<ircs://irc.libera.chat:6697/#mcfproj>
