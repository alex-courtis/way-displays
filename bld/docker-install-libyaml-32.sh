#
# Install packages within the docker container.
# Not for running locally.
#

#
# 32 bit libyaml to /usr/lib32
#
mkdir -p lib32
curl -L https://github.com/yaml/libyaml/releases/download/0.2.5/yaml-0.2.5.tar.gz | tar zx --directory lib32

cd lib32/yaml-0.2.5

CFLAGS=-m32 ./configure --prefix=/usr --libdir=/usr/lib32

cd src

make

make install
