# Building

This document is intended for mediasoup developers.

## NPM scripts

The `package.json` file in the main folder includes the following scripts:

### `npm run typescript:build`

Compiles mediasoup TypeScript code (`node/src` folder) JavaScript and places it into the `node/lib` directory.

### `npm run typescript:watch`

Compiles mediasoup TypeScript code (`node/src` folder) JavaScript, places it into the `node/lib` directory an watches for changes in the TypeScript files.

### `npm run worker:build`

Builds the `mediasoup-worker` binary. It invokes `invoke`below.

### `npm run worker:prebuild-name`

Prints the name of the corresponding `mediasoup-worker` prebuild tar file.

### `npm run worker:prebuild`

Creates a prebuilt of `mediasoup-worker` binary in the `worker/prebuild` folder.

### `npm run lint`

Runs both `lint:node` and `lint:worker` tasks.

### `npm run lint:node`

Validates mediasoup TypeScript files using [ESLint](https://eslint.org), [Prettier](https://prettier.io) and [Knip](https://knip.dev/).

### `npm run lint:worker`

Validates mediasoup worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html). It invokes `invoke lint` below.

See [Install clang-format](#install-clang-format) for requirements.

### `npm run format`

Runs both `format:node` and `format:worker` tasks.

### `npm run format:node`

Format TypeScript and JavaScript code using [Prettier](https://prettier.io).

### `npm run format:worker`

Rewrites mediasoup worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html). It invokes `invoke format` below.

See [Install clang-format](#install-clang-format) for requirements.

### `npm run tidy:worker`

Runs [clang-tidy](http://clang.llvm.org/extra/clang-tidy) and performs C++ code checks following `worker/.clang-tidy` rules. It invokes `invoke tidy` below.

See [Install clang-tidy](#install-clang-tidy) for requirements.

### `npm run tidy:worker:fix`

Same as `npm run tidy:worker` but it also applies fixes.

### `npm run flatc`

Runs both `flatc:node` and `flatc:worker` tasks.

### `npm run flatc:node`

Compiles [FlatBuffers](https://github.com/google/flatbuffers) `.fbs` files in `worker/fbs` to TypeScript code.

### `npm run flatc:worker`

Compiles [FlatBuffers](https://github.com/google/flatbuffers) `.fbs` files in `worker/fbs` to C++ code.

### `npm run test`

Runs both `test:node` and `test:worker` tasks.

### `npm run test:node`

Runs [Jest](https://jestjs.io) test units located at `node/test` folder.

Jest command arguments can be given using `--` as follows:

```bash
npm run test:node -- --testPathPatterns "node/src/test/test-Worker.ts" --testNamePattern "createWorker"
```

### `npm run test:worker`

Runs [Catch2](https://github.com/catchorg/Catch2) test units located at `worker/test` folder. It invokes `invoke test` below.

### `npm run coverage`

Runs `coverage:node` task.

### `npm run coverage:node`

Same as `test:node` task but it also opens a browser window with TypeScript coverage results.

### `npm run release:check`

Runs linters and tests in Node and C++ code. Also verifies that `CHANGELOG.md` has an entry matching the `mediasoup` version in `package.json`.

### `npm run release x.y.z`

Prepares and triggers the release of a new NPM version "x.y.z" of mediasoup. The actual GitHub release and NPM publish are done by CI (`mediasoup-npm-publish.yaml`) once the pushed tag arrives. It:

- Performs checks (lint + test + build + publish dry-run + `CHANGELOG.md` entry check). It runs before the version bump, so the CHANGELOG check validates the previous version's entry (still in package.json), which is harmless.
- Bumps the version to "x.y.z" in `package.json` and `package-lock.json` with `npm version x.y.z --no-git-tag-version`, and sets the top `### NEXT` heading of `CHANGELOG.md` to `### x.y.z`.
- Commits the bump, creates the "x.y.z" tag, and pushes the branch and the tag.

Requirements for it to work:

- Must be called with a SEMVER version as single argument.
- Must be in the main branch.
- Work tree must be clean.
- Changes for the new version must be under the `### NEXT` heading in `CHANGELOG.md`.

### `npm run release:rust:check`

Dry-run of the Rust release. Always runs the `cargo fmt`, `cargo clippy`, `cargo test` and `cargo doc` checks (even when there is nothing to publish). Then checks which of the three Rust crate versions (`mediasoup-sys` in `worker/Cargo.toml`, `mediasoup-types` in `rust/types/Cargo.toml` and `mediasoup` in `rust/Cargo.toml`) are not yet published on crates.io, verifies that `rust/CHANGELOG.md` has an entry matching the `mediasoup` crate version (when that crate is going to be published), and reports what would be done without creating any tag/release or publishing anything.

### `npm run release:rust`

Runs the same checks as `release:rust:check` (including the cargo `fmt`, `clippy`, `test` and `doc` checks), then pushes local commits to GitHub (so everything that gets tagged and published is already there) and publishes to crates.io (with `cargo publish --locked`, in dependency order: `mediasoup-types`, `mediasoup-sys`, `mediasoup`) every Rust crate whose version is not yet on crates.io. Additionally, when the `mediasoup` crate itself is being published, it also creates the Git tag (`rust-X.X.X`, matching its version in `rust/Cargo.toml`) and the corresponding GitHub release (see `release:rust:check` above and `doc/Rust-crates.md`). Requirements for it to work:

- Must be in the main branch.
- Git local repository must be clean. No pending commits or dirty status.
- `Cargo.lock` must be in sync (run `cargo build` and commit it if needed); otherwise the release aborts before doing anything irreversible.
- When the `mediasoup` crate version is incremented, `rust/CHANGELOG.md` must have been updated with an entry matching it.
- A `GITHUB_TOKEN` environment variable with permissions to create releases in GitHub is required (only when the `mediasoup` crate is published).
- Of course, permissions to publish in crates.io are required.

## Rust

The only special feature in Rust case is special environment variable "MEDIASOUP_LOCAL_DEV", that when set to "true" will allow incremental recompilation of changed C++ sources during hacking on mediasoup.

It is not necessary for normal usage of mediasoup as a dependency.

Other than that and normal `cargo` commands, see `npm run release:rust:check` and `npm run release:rust` above.

## Python Invoke and `tasks.py` file

mediasoup uses Python [Invoke](https://www.pyinvoke.org) library for managing and organizing tasks in the `worker` folder (mediasoup worker C++ subproject). `Invoke` is basically a replacemente of `make` + `Makefile` written in Python. mediasoup automatically installs `Invoke` in a local custom path during the installation process (in both Node and Rust) so the user doesn't need to worry about it.

Tasks are defined in `worker/tasks.py`. For development purposes, developers or contributors can install `Invoke` using `pip3 install invoke` and run tasks below within the `worker` folder.

See all the tasks by running `invoke --list` within the `worker` folder.

_NOTE:_ For some of these tasks to work, npm dependencies of `worker/scripts/package.json` must be installed:

```bash
npm ci --prefix worker/scripts
```

### `invoke` (default task)

Alias of `invoke mediasoup-worker` task below.

### `invoke meson-ninja`

Installs `meson` and `ninja` into a local custom path.

### `invoke clean`

Cleans built objects and binaries.

### `invoke clean-build`

Cleans built objects and other artifacts, but keeps `mediasoup-worker` binary in place.

### `invoke clean-pip`

Cleans `meson` and `ninja` installed in local prefix with pip.

### `invoke clean-subprojects`

Cleans subprojects downloaded with Meson.

### `invoke clean-all`

Cleans built objects and binaries, `meson` and `ninja` installed in local prefix with pip and all subprojects downloaded with Meson.

### `invoke check-wrap-status`

Check the status of the Meson subprojects. It also prints whether there are updates available.

### `invoke update-wrap-file [subproject]`

Updates the wrap file of a Meson subproject (those in `worker/subprojects` folder). After updating it, `invoke setup` must be called by passing `MESON_ARGS="--reconfigure"` environment variable. Usage example:

```bash
cd worker
invoke update-wrap-file openssl
MESON_ARGS="--reconfigure" invoke setup
```

### `invoke mediasoup-worker`

Builds the `mediasoup-worker` binary at `worker/out/Release`.

If the "MEDIASOUP_MAX_CORES" environment variable is set, the build process will use that number of CPU cores. Otherwise it will auto-detect the number of cores in the machine.

"MEDIASOUP_BUILDTYPE" environment variable controls build types, "Release" and "Debug" are presets optimized for those use cases. Other build types are possible too, but they are not presets and will require "MESON_ARGS" use to customize build configuration.

Check the meaning of useful macros in the `worker/include/Logger.hpp` header file if you want to enable tracing or other debug information.

Binary is built at `worker/out/MEDIASOUP_BUILDTYPE/build`.

In order to instruct the mediasoup Node.js module to use the "Debug" mediasoup-worker` binary, an environment variable must be set before running the Node.js application:

```bash
MEDIASOUP_BUILDTYPE=Debug node myapp.js
```

If the "MEDIASOUP_WORKER_BIN" environment variable is set (it must be an absolute file path), mediasoup will use the it as `mediasoup-worker` binary and **won't** compile the binary:

```bash
MEDIASOUP_WORKER_BIN="/home/xxx/src/foo/mediasoup-worker" node myapp.js
```

### `invoke libmediasoup-worker`

Builds the `libmediasoup-worker` static library at `worker/out/Release`.

"MEDIASOUP_MAX_CORES"` and "MEDIASOUP_BUILDTYPE" environment variables from above still apply for static library build.

### `invoke xcode`

Builds a Xcode project for the mediasoup worker subproject.

### `invoke lint`

Validates mediasoup worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html) and rules in `worker/.clang-format`. It also validates Python code within `worker` folder (i.e. `worker/tasks.py` file).

**Requirements:**

- A specific version of `clang-format`is required. See [Install clang-format](#install-clang-format).
- `clang-format-VERSION` or `clang-format` (corresponding to the required version) must be in the `PATH`. If not, add it before running the command.

### `invoke format`

Rewrites mediasoup worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html).

**Requirements:**

- A specific version of `clang-format`is required. See [Install clang-format](#install-clang-format).
- `clang-format-VERSION` or `clang-format` (corresponding to the required version) must be in the `PATH`. If not, add it before running the command.

### `invoke tidy`

Runs [clang-tidy](http://clang.llvm.org/extra/clang-tidy) and performs C++ code checks following `worker/.clang-tidy` rules.

**Requirements:**

- `invoke clean` must have been called first.
- A specific version of `clang-tidy`is required. See [Install clang-tidy](#install-clang-tidy).
- `clang-tidy-VERSION` or `clang-tidy` (corresponding to the required version) must be in the `PATH`. If not, add it before running the command. Same for other `clang-tidy` related executables such as `run-clang-tidy` and `clang-apply-replacements`,

**Environment variables:**

- "MEDIASOUP_TIDY_CHECKS": Optional. Comma separated list of checks. Overrides the checks defined in `worker/.clang-tidy` file.
- "MEDIASOUP_TIDY_FILES": Optional. Space separated source file paths to process. All `.cpp` files will be processes by default.
  - File paths must be relative to `worker/` folder.
  - File paths can use [glob](https://github.com/isaacs/node-glob) syntax. Example: `"src/RTC/SCTP/**/*.cpp"`.

**Usage example in macOS:**

```bash
PATH="/opt/homebrew/opt/llvm/bin/:$PATH" invoke tidy
```

It may happens that `clang-tidy` doesn't know where C++ standard libraries are so it shows lot of warnings about them. Depending on your local setup this may work:

```bash
PATH="/opt/homebrew/opt/llvm/bin/:$PATH" CPATH=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1 invoke tidy
```

### `invoke tidy-fix`

Same as `invoke tidy` but it also applies fixes.

### `invoke test`

Builds and runs the `mediasoup-worker-test` binary at `worker/out/Release` (or at `worker/out/Debug` if the "MEDIASOUP_BUILDTYPE" environment variable is set to "Debug"), which uses [Catch2](https://github.com/catchorg/Catch2) to run test units located at `worker/test` folder.

### `invoke test-asan-address`

Run test with Address Sanitizer with `-fsanitize=address`.

### `invoke test-asan-undefined`

Run test with Address Sanitizer with `-fsanitize=undefined`.

### `invoke fuzzer`

Builds the `mediasoup-worker-fuzzer` binary (which uses [libFuzzer](http://llvm.org/docs/LibFuzzer.html)) at `worker/out/Release` (or at `worker/out/Debug/` if the "MEDIASOUP_BUILDTYPE" environment variable is set to "Debug").

**Requirements:**

- Linux with fuzzer capable clang++.
- "CC" environment variable must point to `clang`.
- "CXX" environment variable must point to `clang++`.

Read the [Fuzzer](Fuzzer.md) documentation for detailed information.

### `invoke fuzzer-run-all`

Runs all fuzzer cases.

### `invoke docker`

Builds a Linux Ubuntu Docker image with fuzzer capable clang++ and all dependencies to run mediasoup.

### `invoke docker-run`

Runs a container of the Ubuntu Docker image created with `invoke docker`. It automatically executes a `bash` session in the mediasoup directory, which is a Docker volume that points to the mediasoup root folder.

**NOTE:** To install and run mediasoup in the container, previous installation (if any) must be properly cleaned by entering the `worker` directory and running `invoke clean-all`.

### `invoke docker-alpine`

Builds a Linux Alpine Docker image with all dependencies to run mediasoup.

### `invoke docker-alpine-run`

Runs a container of the Alpine Docker image created with `invoke docker-alpine`. It automatically executes an `ash` session in the mediasoup directory, which is a Docker volume that points to the mediasoup root folder.

**NOTE:** To install and run mediasoup in the container, previous installation (if any) must be properly cleaned by entering the `worker` directory and running `invoke clean-all`.

### `invoke docker-386`

Builds a 386 Linux Debian (32 bits arch) Docker image with all dependencies to run mediasoup.

### `invoke docker-alpine-386`

Runs a container of the 386 Linux Debian (32 bits arch) Docker image created with `invoke docker-386`. It automatically executes an `ash` session in the mediasoup directory, which is a Docker volume that points to the mediasoup root folder.

**NOTE:** To install and run mediasoup in the container, previous installation (if any) must be properly cleaned by entering the `worker` directory and running `invoke clean-all`.
**NOTE:** Due to the very old Node v18 in this image, in order to run mediasoup Node tests, `npm ci` must be executed with `--ignore-scripts --engine-strict=false` arguments.

## Makefile

The `worker` folder contains a `Makefile` file for the mediasoup worker C++ subproject. It acts as a proxy to the `Invoke` tasks defined in `tasks.py`. The `Makefile` file exists to help developers or contributors that prefer keep using `make` commands.

All tasks defined in `tasks.py` (see above) are available in `Makefile`. There is only one exception:

- The `update-wrap-file` needs a "SUBPROJECT" environment variable indicating the subproject to update. Usage example:
  ```bash
  cd worker
  make update-wrap-file SUBPROJECT=openssl
  ```

## Install clang-format

A specific `clang-format` version is required to be installed in the system, which is defined in [clang-scripts.mjs](../worker/scripts/clang-scripts.mjs).

macOS:

```bash
brew install clang-format@VERSION
```

Linux:

```bash
apt-get install clang-format-VERSION
```

## Install clang-tidy

A specific `clang-tidy` version is required to be installed in the system, which is defined in [clang-scripts.mjs](../worker/scripts/clang-scripts.mjs).

macOS:

```bash
brew install clang-tidy@VERSION
```

Linux:

```bash
apt-get install clang-tidy-VERSION
```
