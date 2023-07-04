FROM amd64/archlinux:base-devel

RUN pacman -Sy clang wayland-protocols wlroots yaml-cpp cmocka cppcheck --noconfirm

CMD ["/usr/bin/bash"]
