#
# Install packages within the docker container.
# Not for running locally.
#

VERSION="0.26"

# speed up builds a bit
export MAKEFLAGS=-j8

mkdir -p /src
cd /src

#
# include-what-you-use to /usr/local
#
curl -L "https://github.com/include-what-you-use/include-what-you-use/archive/refs/tags/${VERSION}.tar.gz" | tar --no-same-owner -zx

cd "include-what-you-use-${VERSION}"

cmake -Bbuild --install-prefix /usr/local

cmake --build build

cmake --install build
