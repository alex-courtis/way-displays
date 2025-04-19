# docker image for local dev
# keep this in sync with ci.yml

FROM archlinux:multilib-devel

COPY .github/workflows/arch-packages.sh /usr/local/bin

RUN arch-packages.sh

ENV DEBUGINFOD_URLS="https://debuginfod.archlinux.org"
ENV DEBUGINFOD_CACHE_PATH="/tmp/debuginfod_client"

ENTRYPOINT [ "sleep", "infinity" ]
