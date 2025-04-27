# docker image for local dev
# keep this in sync with ci.yml

FROM archlinux:multilib-devel

COPY .github/workflows/packages/base/install.sh /usr/local/bin/packages-base-install.sh

RUN packages-base-install.sh

ENV DEBUGINFOD_URLS="https://debuginfod.archlinux.org"
ENV DEBUGINFOD_CACHE_PATH="/tmp/debuginfod_client"

ENTRYPOINT [ "sleep", "infinity" ]
