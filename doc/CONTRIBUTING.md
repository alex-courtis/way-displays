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

### Developing On The (CI) Arch Image

`Dockerfile` defines an image similar to that used by the docker container in `ci.yml`

It is intended to run in detached mode, thus the `ENTRYPOINT [ "sleep", "infinity" ]`

Build the image:
```sh
make docker-build
```

Run a detached container:
```sh
make docker-run
```

Build and install the AUR lib32-yaml-cpp and include-what-you-use packages:
```sh
make docker-packages
```

Execute a command in the container e.g.:
```sh
docker exec way-displays make MFLAGS=-m32 clean test-vg
```

OR run a shell in the container:
```sh
docker exec -it way-displays /bin/bash
```

Stop and remove the container:
```sh
make docker-rm
```

Remove the image:
```sh
make docker-image-rm
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

