#!/bin/bash -e

_version=2.3.1
_arch=$(dpkg --print-architecture)

wget -c "https://github.com/mongodb/mongo-c-driver/releases/download/${_version}/mongo-c-driver-${_version}.tar.gz"
echo "8d52d089815356e4c74b16a00239ee19ee5f5418381a6b7947cbe34a6c838613"   \
  "mongo-c-driver-${_version}.tar.gz" | sha256sum -c

mkdir -p "build_makedeb_mongo-c-driver"
tar -xzf "mongo-c-driver-${_version}.tar.gz" -C "build_makedeb_mongo-c-driver"

cmake -G Ninja  \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo  \
  -DENABLE_ZSTD=ON -DENABLE_ZLIB=SYSTEM  \
  -DMONGOC_INSTALL_INCLUDEDIR="include"  \
  -DBSON_INSTALL_INCLUDEDIR="include"  \
  -DCMAKE_INSTALL_PREFIX="/usr/local"  \
  -S "build_makedeb_mongo-c-driver/mongo-c-driver-${_version}"  \
  -B "build_makedeb_mongo-c-driver/build"

cmake --build "build_makedeb_mongo-c-driver/build"

rm -rf "build_makedeb_mongo-c-driver/pkg"
mkdir -p "build_makedeb_mongo-c-driver/pkg/DEBIAN"

DESTDIR=$(realpath "build_makedeb_mongo-c-driver/pkg") cmake --install  \
  "build_makedeb_mongo-c-driver/build"

tee "build_makedeb_mongo-c-driver/pkg/DEBIAN/control" <<control_EOF
Package: mongo-c-driver-local
Provides: libbson-dev, libmongoc-dev
Version: ${_version}
Architecture: ${_arch}
Multi-Arch: same
Maintainer: LH_Mouse <lh_mouse@126.com>
Homepage: https://github.com/mongodb/mongo-c-driver
License: Apache-2.0
Description: MongoDB client library
control_EOF

tee "build_makedeb_mongo-c-driver/pkg/DEBIAN/postinst" <<postinst_EOF
#!/bin/sh -e
ldconfig
postinst_EOF

chmod +x "build_makedeb_mongo-c-driver/pkg/DEBIAN/postinst"
cp -p "build_makedeb_mongo-c-driver/pkg/DEBIAN/"post{inst,rm}

dpkg-deb --root-owner-group --build "build_makedeb_mongo-c-driver/pkg"  \
  "mongo-c-driver-local_${_version}_${_arch}.deb"
