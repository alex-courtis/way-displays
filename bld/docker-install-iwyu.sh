#
# Install packages within the docker container.
# Not for running locally.
#

# speed up builds a bit
export MAKEFLAGS=-j8

#
# include-what-you-use to /usr/local
#
mkdir -p iwyu
curl -L https://github.com/include-what-you-use/include-what-you-use/archive/refs/tags/0.26.tar.gz | tar zx --directory iwyu

cd iwyu/include-what-you-use-0.26

cmake -Bbuild --install-prefix /usr/local

cmake --build build

cmake --install build
