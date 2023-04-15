#!/bin/bash

sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo
sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo
yum clean all
yum list installed
yum install -y devtoolset-10-libasan-devel devtoolset-10-liblsan-devel devtoolset-10-libtsan-devel devtoolset-10-libubsan-devel
