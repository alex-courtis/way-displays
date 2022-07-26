# Contributing to https://github.com/alex-courtis/way-displays

Thank you for contributing.

Ideas, bug fixes and enhancements are always welcome.

Please raise an [issue](https://github.com/alex-courtis/way-displays/issues), fork the repository and raise a [PR](https://github.com/alex-courtis/way-displays/pulls).

## Documentation

Please update `README.md` and `doc/configuration.md`.

Please update the man page:
* update `way-displays.1.pandoc`
* run `make man`
  * preview the man page via `man -l way-displays.1`
* commit both `way-displays.1.pandoc` and `way-displays.1`

## Styling and Linting

Please match the style of the surrounding code and obey `.editorconfig`. Default vim C-indenting `gg=G` is preferred.

Please run `make cppcheck` and resolve all issues before committing.

## Adding Options

Please add the option to `config.yaml` with a descriptive comment.

Please add a command line option and update the usage message.

