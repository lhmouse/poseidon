#!/bin/bash -e

_pkgname=poseidon
_pkgversion=$(git describe --tag | sed 's/^[^0-9]//')
_pkgarch=$(dpkg --print-architecture)
_tempdir=$(readlink -f "./.makedeb")

rm -rf ${_tempdir}
mkdir -p ${_tempdir}
cp -pr DEBIAN -t ${_tempdir}

meson setup -Dbuildtype=release build_release
meson compile -Cbuild_release
meson test -Cbuild_release
DESTDIR=${_tempdir} meson install -Cbuild_release

_etcdir=$(find ${_tempdir} -name "etc" -type d)
find ${_etcdir} -type f | sed 's|^.*\.makedeb/|/|' > ${_tempdir}/conffiles

sed -i "s/{_pkgname}/${_pkgname}/" ${_tempdir}/DEBIAN/control
sed -i "s/{_pkgversion}/${_pkgversion}/" ${_tempdir}/DEBIAN/control
sed -i "s/{_pkgarch}/${_pkgarch}/" ${_tempdir}/DEBIAN/control

dpkg-deb --root-owner-group --build .makedeb "${_pkgname}_${_pkgversion}_${_pkgarch}.deb"
