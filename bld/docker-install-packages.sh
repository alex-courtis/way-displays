#
# Install packages within the docker container.
# Not for running locally.
#

# speed up builds a bit
export MAKEFLAGS=-j8

# 
# arch 64 and 32 bit packages
#
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


#
# include-what-you-use to /usr/local
#
mkdir -p iwyu
curl -L https://github.com/include-what-you-use/include-what-you-use/archive/refs/tags/0.26.tar.gz | tar zx --directory iwyu

cd iwyu/include-what-you-use-0.26

cmake -Bbuild --install-prefix /usr/local

cmake --build build

cmake --install build

cd


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
