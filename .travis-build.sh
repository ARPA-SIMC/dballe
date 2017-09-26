#!/bin/bash
set -ex

image=$1

if [[ $image =~ ^centos: ]]
then
    pkgcmd="yum"
    builddep="yum-builddep"
    yum install -y yum-utils
elif [[ $image =~ ^fedora: ]]
then
    pkgcmd="dnf"
    builddep="dnf builddep"
    dnf install -y @buildsys-build 'dnf-command(builddep)' fedora-packager
    dnf copr enable -y pat1/simc
fi

$builddep -y fedora/SPECS/dballe.spec

if [[ $image =~ "^fedora:" ]]
then
    pkgname="$(rpmspec -q --qf="dballe-%{version}-%{release}\n" fedora/SPECS/dballe.spec | head -n1)"
    rpmdev-setuptree
    git archive --prefix=$pkgname/ --format=tar HEAD | gzip -c > ~/rpmbuild/SOURCES/$pkgname.tar.gz
    rpmbuild -ba fedora/SPECS/dballe.spec
    find ~/rpmbuild/{RPMS,SRPMS}/ -name "${pkgname}*rpm" -exec cp -v {} . \;
    # TODO upload ${pkgname}*.rpm to github release on deploy stage
else
    autoreconf -ifv
    ./configure
    make check
fi
