#
# Install packages within the docker container.
# Not for running locally.
#

# 
# arch 64 and 32 bit packages
#
pacman --noconfirm -Syu \
	clang \
	cmake \
	cmocka lib32-cmocka \
	cppcheck \
	git \
	lib32-gcc-libs \
	lib32-systemd \
	libinput \
	libyaml \
	llvm \
	valgrind \
	wayland lib32-wayland \
	wayland-protocols
