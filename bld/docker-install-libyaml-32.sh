#
# Install packages within the docker container.
# Not for running locally.
#

VERSION="0.2.5"

#
# 32 bit libyaml to /usr/lib32
#
curl -L "https://github.com/yaml/libyaml/releases/download/${VERSION}/yaml-${VERSION}.tar.gz" | tar zx

cd "yaml-${VERSION}"

CFLAGS=-m32 ./configure --prefix=/usr --libdir=/usr/lib32

cd src

make

make install
