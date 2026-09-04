#!/bin/bash -e

_version=9.7.2
_md5='dd0a38f0e4187f0f096964363c37c4ba'
_arch=$(dpkg --print-architecture)

# https://dev.mysql.com/downloads/mysql/
wget -c "https://dev.mysql.com/get/Downloads/MySQL-${_version%.*}/mysql-${_version}.tar.gz"
echo "${_md5}" "mysql-${_version}.tar.gz" | md5sum -c

mkdir -p "build_makedeb_mysql-client"
tar -xzf "mysql-${_version}.tar.gz" -C "build_makedeb_mysql-client"

cmake -G Ninja  \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo  \
  -DCMAKE_INSTALL_PREFIX="/usr/local" -DINSTALL_INCLUDEDIR="include/mysql"  \
  -DWITH_SYSTEM_LIBS=ON -DWITHOUT_SERVER=ON  \
  -DMYSQL_UNIX_ADDR="/var/run/mysqld/mysqld.sock"  \
  -S "build_makedeb_mysql-client/mysql-${_version}"  \
  -B "build_makedeb_mysql-client/build"

cmake --build "build_makedeb_mysql-client/build"

rm -rf "build_makedeb_mysql-client/pkg"
mkdir -p "build_makedeb_mysql-client/pkg/DEBIAN"

DESTDIR=$(realpath "build_makedeb_mysql-client/pkg") cmake --install  \
  "build_makedeb_mysql-client/build"

tee "build_makedeb_mysql-client/pkg/DEBIAN/control" <<control_EOF
Package: mysql-client-local
Provides: libmysqlclient-dev
Version: ${_version}
Architecture: ${_arch}
Multi-Arch: same
Maintainer: LH_Mouse <lh_mouse@126.com>
Homepage: https://downloads.mysql.com/archives/community
License: GPL-2.0-only
Description: MySQL client library and programs
control_EOF

tee "build_makedeb_mysql-client/pkg/DEBIAN/postinst" <<postinst_EOF
#!/bin/sh -e
ldconfig
postinst_EOF

chmod +x "build_makedeb_mysql-client/pkg/DEBIAN/postinst"
cp -p "build_makedeb_mysql-client/pkg/DEBIAN/"post{inst,rm}

dpkg-deb --root-owner-group --build "build_makedeb_mysql-client/pkg"  \
  "mysql-client-local_${_version}_${_arch}.deb"
