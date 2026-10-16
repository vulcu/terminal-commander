# Contributing

Feature contributions for [specific issues](https://github.com/vulcu/terminal-commander/issues) _may_ be accepted, but **only** with prior discussion on the relevant issue page.
Pull requests opened for features not previously discussed will likely be rejected, even if relevant to open issues.

All other contributions (fix, doc, etc.) can be submitted freely, but do require a pull request. Only rebase merges are supported.

**This project prioritizes computational efficiency and minimal memory size for use with embedded systems.** All contributions must also prioritize this.
Contributions which significantly increase memory size or computational overhead will be automatically rejected.

All contributions must be Arduino compatible, and contributions requiring external Arduino or C/C++ libraries will be rejected.

## Getting Started

This project is built with Arduino IDE 2.3.4 or later.

Recommended: Install the [Arduino 2.x IDE](https://github.com/arduino/arduino-ide/releases)
and follow their [README](https://github.com/arduino/arduino-ide#download) to install the latest IDE version.

Clone the repository to the directory of your choice
```sh
git submodule add https://github.com/vulcu/terminal-commander.git 
# or
git submodule add git@github.com:vulcu/terminal-commander.git

# followed by
git submodule --init
```

### Linting

PRs will not merge if they fail linting.
To confirm linting locally, run:

```sh
# linting is not currently enforced for this project
```

## Adding files

All code should be added to the `terminal-commander/src/` directory.

### Documentation

- All commits require a message, and this message must adhere to [Conventional Commit](https://www.conventionalcommits.org/en/v1.0.0/#summary) formatting.
- All pull requests require a summary of changes and reason for changes.
- All contributions require written documentation or will be automatically rejected.
- Written documentation should be in the form of code comments, `*.md` markdown files conforming this [style guide](https://google.github.io/styleguide/docguide/style.html), or both.

#### Including Images in Documentation

**Images should be specific, and relevant to something that cannot be conveyed with text.** Images do not have to be optimized, however they should be of a reasonable filesize (2-5 MB is ideal).

- Add images to the `img` directory in this format: `terminal-commander/docs/img/[image-name][-##].jpg`
- Each image must have [alt text](https://www.w3.org/WAI/tutorials/images/informative/#example-2-images-used-to-supplement-other-information).
- Images may optionally have a photographer, source, and/or [license](https://spdx.org/licenses/).

#### Including Examples in Documentation

- Contributions and documentation can (and are encouraged to) include example usage.
- Examples can add memory or computational overhead as needed, but must be capable of running on an Arduino Uno.
- Examples must not be hardware-specific (e.g. no hardcoded pins, use the Arduino platform's hardware abstraction capabilities).
- Examples are located in the `terminal-commander/examples` directory, **and must demonstrate something which cannot otherwise be demonstrated by updating an existing example**.
- Examples should be kept as generic as possible, with no external dependencies.
