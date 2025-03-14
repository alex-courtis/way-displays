# Contributing to https://github.com/alex-courtis/way-displays

Thank you for contribution!

Ideas, bug fixes and enhancements are always welcome.

Please raise an [issue](https://github.com/alex-courtis/way-displays/issues), fork the repository and raise a [PR](https://github.com/alex-courtis/way-displays/pulls).

## Dependencies

* GNU make
* gcc or clang
* wayland
* wayland-protocols
* wlroots
* libinput
* yaml-cpp

Most will be available if you are running a wlroots based compositor like sway.

yaml-cpp will need to be installed via your distribution's package manager.

## Development

gcc is the default for packaging reasons, however clang is preferred.

Set CC and CXX when invoking make:

`make CC=clang CXX=clang++ ...`

[ccls](https://github.com/MaskRay/ccls) using clang is configured via `.ccls`, for editors that support the [Language Server Protocol](https://microsoft.github.io/language-server-protocol/).

### Compile

`make`

### Test

`make test`

[cmocka](https://cmocka.org/) is used for unit testing. Individual tests with `--wrap` definitions are defined in `tst/GNUmakefile`.

Please add tests when defining new functionality.

Individual tests may be run via test-name e.g.
`make test-cfg`

Valgrind test by appending `-vg` e.g.
`make test-vg`
`make test-cfg-vg`

### Lint

`make cppcheck`

Please resolve all issues before committing.

### Check Includes

[include-what-you-use](https://include-what-you-use.org/) is configured to run for `src` and `tst`.

`make iwyu`

Necessary changes will be indicated in the output with "should".

See all violations:

`make -k iwyu > /dev/null`

### 32-bit

Compilation can be done without 32-bit libs:
```sh
make CC=gcc   CXX=g++     MFLAGS=-m32 clean compile
make CC=clang CXX=clang++ MFLAGS=-m32 clean compile
```

Testing requires 32-bit libs, libinput is excluded:
```sh
make CC=gcc   CXX=g++     MFLAGS=-m32 clean test
make CC=gcc   CXX=g++     MFLAGS=-m32 clean test-vg
make CC=clang CXX=clang++ MFLAGS=-m32 clean test
make CC=clang CXX=clang++ MFLAGS=-m32 clean test-vg
```

### Compiling On Ubuntu Docker Image

This may be necessary to keep compatibily with the github CI docker container.

```sh
docker stop wd_ubuntu
docker rm wd_ubuntu
docker run \
    --detach \
    --interactive \
    --name wd_ubuntu \
    --volume "$(pwd):/way-displays" \
    ubuntu:latest 
docker exec --interactive --tty wd_ubuntu /bin/bash
```

```sh
export DEBIAN_FRONTEND="noninteractive"
apt update -y
apt install -y make gcc gcc-multilib g++ g++-multilib clang wayland-protocols libwlroots-dev libyaml-cpp-dev libcmocka-dev cppcheck valgrind
exit
```

```sh
docker exec --interactive --tty --user "$(id -u):$(id -g)" wd_ubuntu /bin/bash
cd /way-displays
make
```

## Documentation

Please update `README.md`.

Please update the man page:
* update `way-displays.1.pandoc`
* run `make man`
  * preview the man page via `man -l way-displays.1`
* commit both `way-displays.1.pandoc` and `way-displays.1`

## Style

Please match the style of the surrounding code and obey `.editorconfig`. Default vim C-indenting `gg=G` is preferred.

## Adding Options

Please add the option to `cfg.yaml` with a descriptive comment.

Please add a command line option and update the usage message.

