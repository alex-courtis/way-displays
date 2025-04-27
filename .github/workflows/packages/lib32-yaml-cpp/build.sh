#!/bin/sh

# must be run as a user

rm -rf /tmp/lib32-yaml-cpp

git clone https://aur.archlinux.org/lib32-yaml-cpp.git /tmp/lib32-yaml-cpp

makepkg --nocheck OPTIONS=-debug PKGEXT='.pkg.tar' --dir /tmp/lib32-yaml-cpp
