FROM archlinux:multilib-devel

COPY .github/workflows/arch-packages.sh /

RUN /arch-packages.sh
