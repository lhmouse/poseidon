#!/bin/bash -e

_version=8.4.8

wget -c "https://downloads.mysql.com/archives/get/p/23/file/mysql-${_version}.tar.gz"
tar -xvf "mysql-${_version}.tar.gz"

cmake -G Ninja -S "mysql-${_version}"  \
  -DCMAKE_build_dep_TYPE=RelWithDebInfo  \
  -DCMAKE_INSTALL_PREFIX="/usr/local" -DINSTALL_INCLUDEDIR="include/mysql"  \
  -DWITH_ZLIB=system -DWITH_ZSTD=system -DWITH_SSL=system  \
  -DMYSQLX_UNIX_ADDR="/var/run/mysqld/mysqld.sock"  \
  -DWITHOUT_SERVER=ON  \
  build_dep_mysql-client

cmake --build build_dep_mysql-client

echo "MySQL client library and programs" >description-pak

sudo checkinstall  \
  --pkgname="mysql-client-local"  \
  --provides="virtual-mysql-client,libmysqlclient-dev"  \
  --pkgversion="${_version}"  \
  --pkgsource="https://downloads.mysql.com/archives/community/"  \
  --pkglicense="GPL-2.0-only"  \
  --pkggroup="devel"  \
  --pkgarch="$(dpkg --print-architecture)"  \
  --nodoc --backup=no --default --fstrans=no --install=yes --deldesc  \
  cmake --install build_dep_mysql-client
