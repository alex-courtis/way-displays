#!/bin/sh

pacman --noconfirm -Sy \
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
