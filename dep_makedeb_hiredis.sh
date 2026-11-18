#!/bin/bash -e

_version=1.4.1
_sha256='ca3180359a8b1275838a45415851f8cd5c411e27bdbf18f4823012e45507d2e4'
_arch=$(dpkg --print-architecture)

# https://github.com/redis/hiredis/releases/
wget -c "https://github.com/redis/hiredis/archive/refs/tags/v${_version}.tar.gz"  \
    -O "hiredis-${_version}.tar.gz"
echo "${_sha256}" "hiredis-${_version}.tar.gz" | sha256sum -c

mkdir -p "build_makedeb_hiredis"
tar -xzf "hiredis-${_version}.tar.gz" -C "build_makedeb_hiredis"

cmake -G Ninja  \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo  \
  -DCMAKE_INSTALL_PREFIX="/usr/local"  \
  -DENABLE_SSL=ON  \
  -S "build_makedeb_hiredis/hiredis-${_version}"  \
  -B "build_makedeb_hiredis/build"

cmake --build "build_makedeb_hiredis/build"

rm -rf "build_makedeb_hiredis/pkg"
mkdir -p "build_makedeb_hiredis/pkg/DEBIAN"

DESTDIR=$(realpath "build_makedeb_hiredis/pkg") cmake --install  \
  "build_makedeb_hiredis/build"

tee "build_makedeb_hiredis/pkg/DEBIAN/control" <<control_EOF
Package: hiredis-local
Provides: libhiredis-dev
Version: ${_version}
Architecture: ${_arch}
Multi-Arch: same
Maintainer: LH_Mouse <lh_mouse@126.com>
Homepage: https://github.com/redis/hiredis
License: BSD-3-Clause
Description: Minimalistic C client library for Redis
control_EOF

tee "build_makedeb_hiredis/pkg/DEBIAN/postinst" <<postinst_EOF
#!/bin/sh -e
ldconfig
postinst_EOF

chmod +x "build_makedeb_hiredis/pkg/DEBIAN/postinst"
cp -p "build_makedeb_hiredis/pkg/DEBIAN/"post{inst,rm}

dpkg-deb --root-owner-group --build "build_makedeb_hiredis/pkg"  \
  "hiredis-local_${_version}_${_arch}.deb"
