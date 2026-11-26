#!/bin/bash -e

_version=1.5.1
_sha256='f5ec19151c0f4f15ca018c22b17b6f6739a672a4fc0c085b692aef69a154ebef'
_arch=$(dpkg --print-architecture)

# https://github.com/redis/hiredis/releases/
wget -c "https://github.com/redis/hiredis/archive/refs/tags/v${_version}.tar.gz"  \
  -O "hiredis-${_version}.tar.gz"
echo "${_sha256}" "hiredis-${_version}.tar.gz" | sha256sum -c

mkdir -p "build_hiredis"
tar -xzf "hiredis-${_version}.tar.gz" -C "build_hiredis"

cmake -G Ninja  \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo  \
  -DCMAKE_INSTALL_PREFIX="/usr/local"  \
  -DENABLE_SSL=ON  \
  -S "build_hiredis/hiredis-${_version}"  \
  -B "build_hiredis/build"

cmake --build "build_hiredis/build"

rm -rf "build_hiredis/pkg"
mkdir -p "build_hiredis/pkg/DEBIAN"

DESTDIR=$(realpath "build_hiredis/pkg") cmake --install  \
  "build_hiredis/build"

tee "build_hiredis/pkg/DEBIAN/control" <<control_EOF
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

tee "build_hiredis/pkg/DEBIAN/postinst" <<postinst_EOF
#!/bin/sh -e
ldconfig
postinst_EOF

chmod +x "build_hiredis/pkg/DEBIAN/postinst"
cp -p "build_hiredis/pkg/DEBIAN/"post{inst,rm}

dpkg-deb --root-owner-group --build "build_hiredis/pkg"  \
  "hiredis-local_${_version}_${_arch}.deb"
