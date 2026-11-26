#!/bin/bash -e

_version=9.7.4.1
_sha256='5150b1b27dc29073936f228d0c7baf043c085079439aec35a13b06f84a9d1db0'
_arch=$(dpkg --print-architecture)

# https://github.com/mysql/mysql-server/tags/
wget -c "https://github.com/mysql/mysql-server/archive/refs/tags/mysql-${_version}.tar.gz"  \
  -O "mysql-${_version}.tar.gz"
echo "${_sha256}" "mysql-${_version}.tar.gz" | sha256sum -c

mkdir -p "build_mysql-client"
tar -xzf "mysql-${_version}.tar.gz" -C "build_mysql-client"

cmake -G Ninja  \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo  \
  -DCMAKE_INSTALL_PREFIX="/usr/local" -DINSTALL_INCLUDEDIR="include/mysql"  \
  -DWITH_SYSTEM_LIBS=ON -DWITHOUT_SERVER=ON  \
  -DMYSQL_UNIX_ADDR="/var/run/mysqld/mysqld.sock"  \
  -S "build_mysql-client/mysql-server-mysql-${_version}"  \
  -B "build_mysql-client/build"

cmake --build "build_mysql-client/build"

rm -rf "build_mysql-client/pkg"
mkdir -p "build_mysql-client/pkg/DEBIAN"

DESTDIR=$(realpath "build_mysql-client/pkg") cmake --install  \
  "build_mysql-client/build"

tee "build_mysql-client/pkg/DEBIAN/control" <<control_EOF
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

tee "build_mysql-client/pkg/DEBIAN/postinst" <<postinst_EOF
#!/bin/sh -e
ldconfig
postinst_EOF

chmod +x "build_mysql-client/pkg/DEBIAN/postinst"
cp -p "build_mysql-client/pkg/DEBIAN/"post{inst,rm}

dpkg-deb --root-owner-group --build "build_mysql-client/pkg"  \
  "mysql-client-local_${_version}_${_arch}.deb"
