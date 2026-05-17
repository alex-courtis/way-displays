# docker image for local dev
# Keep this in sync with ci.yml
# GH CI doesn't allow use of a Dockerfile, hence we maintain this separately

FROM archlinux:multilib-devel

COPY bld/docker-install-packages.sh /usr/local/bin

RUN . docker-install-packages.sh

ENV DEBUGINFOD_URLS="https://debuginfod.archlinux.org"
ENV DEBUGINFOD_CACHE_PATH="/tmp/debuginfod_client"
