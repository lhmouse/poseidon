#!/bin/bash -e

_version=9.4.4
_sha256='5600adb1498f82a9c719b8eb19310021737388e0d92362a6d47be532710d3c96'
_arch=$(dpkg --print-architecture)

# https://github.com/nodejs/llhttp/releases/
wget -c "https://github.com/nodejs/llhttp/archive/refs/tags/release/v${_version}.tar.gz"  \
  -O "llhttp-${_version}.tar.gz"
echo "${_sha256}" "llhttp-${_version}.tar.gz" | sha256sum -c

mkdir -p "build_llhttp"
tar -xzf "llhttp-${_version}.tar.gz" -C "build_llhttp"

cmake -G Ninja  \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo  \
  -DCMAKE_INSTALL_PREFIX="/usr/local"  \
  -S "build_llhttp/llhttp-release-v${_version}"  \
  -B "build_llhttp/build"

cmake --build "build_llhttp/build"

rm -rf "build_llhttp/pkg"
mkdir -p "build_llhttp/pkg/DEBIAN"

DESTDIR=$(realpath "build_llhttp/pkg") cmake --install  \
  "build_llhttp/build"

tee "build_llhttp/pkg/DEBIAN/control" <<control_EOF
Package: llhttp-local
Provides: libllhttp-dev
Version: ${_version}
Architecture: ${_arch}
Multi-Arch: same
Maintainer: LH_Mouse <lh_mouse@126.com>
Homepage: https://github.com/nodejs/llhttp
License: MIT
Description: Node.js HTTP parser
control_EOF

tee "build_llhttp/pkg/DEBIAN/postinst" <<postinst_EOF
#!/bin/sh -e
ldconfig
postinst_EOF

chmod +x "build_llhttp/pkg/DEBIAN/postinst"
cp -p "build_llhttp/pkg/DEBIAN/"post{inst,rm}

dpkg-deb --root-owner-group --build "build_llhttp/pkg"  \
  "llhttp-local_${_version}_${_arch}.deb"
