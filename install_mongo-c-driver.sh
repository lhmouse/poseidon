#!/bin/bash -e

_version=2.3.1

wget -c "https://github.com/mongodb/mongo-c-driver/releases/download/${_version}/mongo-c-driver-${_version}.tar.gz"
tar -xvf "mongo-c-driver-${_version}.tar.gz"

cmake -G Ninja -S "mongo-c-driver-${_version}"  \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo  \
  -DENABLE_STATIC=ON -DENABLE_SHARED=ON -DENABLE_UNINSTALL=OFF \
  -DENABLE_ZSTD=ON -DENABLE_ZLIB=SYSTEM  \
  -DMONGOC_INSTALL_INCLUDEDIR=include \
  -DBSON_INSTALL_INCLUDEDIR=include \
  -DMONGOC_INSTALL_CMAKEDIR=lib/cmake/mongoc \
  -DBSON_INSTALL_CMAKEDIR=lib/cmake/bson \
  -DCMAKE_INSTALL_PREFIX="/usr/local"  \
  build_mongo-c-driver

cmake --build build_mongo-c-driver

echo "MongoDB client library" >description-pak

sudo checkinstall  \
  --pkgname="mongo-c-driver-local"  \
  --provides="libbson-dev,libmongoc-dev"  \
  --pkgversion="${_version}"  \
  --pkgsource="https://github.com/mongodb/mongo-c-driver/"  \
  --pkglicense="Apache-2.0"  \
  --pkggroup="devel"  \
  --pkgarch="$(dpkg --print-architecture)"  \
  --nodoc --backup=no --default --fstrans=no --install=yes --deldesc  \
  cmake --install build_mongo-c-driver
