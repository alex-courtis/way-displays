#!/bin/sh

# 64 and 32 bit packages
pacman --noconfirm -Syu \
	clang \
	cmake \
	cmocka lib32-cmocka \
	cppcheck \
	git \
	lib32-systemd \
	libinput \
	libyaml \
	llvm \
	ninja \
	valgrind \
	wayland lib32-wayland \
	wayland-protocols

# 32 bit package is not available, build and install to /usr/lib32
mkdir -p lib32
curl -L https://github.com/yaml/libyaml/releases/download/0.2.5/yaml-0.2.5.tar.gz | tar zx --directory lib32
cd lib32/yaml-0.2.5
CFLAGS=-m32 ./configure --prefix=/usr --libdir=/usr/lib32
cd src
make
make install
