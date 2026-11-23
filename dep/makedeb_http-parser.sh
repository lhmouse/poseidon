#!/bin/bash -e

_version=2.9.4-lhmouse.1
_sha256='e42683adfe967476bb3e9bf8a6d5490d058e37f990469d0a1222c077c5d546b7'
_arch=$(dpkg --print-architecture)

# https://github.com/lhmouse/http-parser/tags/
wget -c "https://github.com/lhmouse/http-parser/archive/refs/tags/v${_version}.tar.gz"  \
  -O "http-parser-${_version}.tar.gz"
echo "${_sha256}" "http-parser-${_version}.tar.gz" | sha256sum -c

mkdir -p "build_http-parser"
tar -xzf "http-parser-${_version}.tar.gz" -C "build_http-parser"

meson setup  \
  -Ddebug=true -Doptimization=3  \
  "build_http-parser/build"  \
  "build_http-parser/http-parser-${_version}"

meson compile -C "build_http-parser/build"

rm -rf "build_http-parser/pkg"
mkdir -p "build_http-parser/pkg/DEBIAN"

DESTDIR=$(realpath "build_http-parser/pkg") meson install  \
  -C "build_http-parser/build"

tee "build_http-parser/pkg/DEBIAN/control" <<control_EOF
Package: http-parser-local
Provides: libhttp-parser-dev
Version: ${_version}
Architecture: ${_arch}
Multi-Arch: same
Maintainer: LH_Mouse <lh_mouse@126.com>
Homepage: https://github.com/lhmouse/http-parser
License: MIT
Description: Node.js HTTP parser
control_EOF

tee "build_http-parser/pkg/DEBIAN/postinst" <<postinst_EOF
#!/bin/sh -e
ldconfig
postinst_EOF

chmod +x "build_http-parser/pkg/DEBIAN/postinst"
cp -p "build_http-parser/pkg/DEBIAN/"post{inst,rm}

dpkg-deb --root-owner-group --build "build_http-parser/pkg"  \
  "http-parser-local_${_version}_${_arch}.deb"
