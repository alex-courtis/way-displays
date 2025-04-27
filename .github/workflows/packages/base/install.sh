#!/bin/sh

pacman --noconfirm -Syu \
	clang \
	cmake \
	cmocka lib32-cmocka \
	cppcheck \
	git \
	lib32-systemd \
	libinput \
	llvm \
	ninja \
	valgrind \
	wayland lib32-wayland \
	wayland-protocols \
	yaml-cpp
