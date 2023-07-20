# Building

This document is intended for mediasoup developers.

## NPM scripts

The `package.json` file in the main folder includes the following scripts:

### `npm run typescript:build`

Compiles mediasoup TypeScript code (`lib` folder) JavaScript and places it into the `lib` directory.

### `npm run typescript:watch`

Compiles mediasoup TypeScript code (`lib` folder) JavaScript, places it into the `lib` directory an watches for changes in the TypeScript files.

### `npm run worker:build`

Builds the `mediasoup-worker` binary. It invokes `make`below.

### `npm run worker:prebuild`

Creates a prebuilt of `mediasoup-worker` in the `worker/prebuild` folder.

### `npm run lint`

Runs both `npm run lint:node` and `npm run lint:worker`.

### `npm run lint:node`

Validates mediasoup JavaScript files using [ESLint](https://eslint.org).

### `npm run lint:worker`

Validates mediasoup-worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html). It invokes `make lint` below.

### `npm run format:worker`

Rewrites mediasoup-worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html). It invokes `make format` below.

### `npm run test`

Runs both `npm run test:node` and `npm run test:worker`.

### `npm run test:node`

Runs [Jest](https://jestjs.io) test units located at `test/` folder.

### `npm run test:worker`

Runs [Catch2](https://github.com/catchorg/Catch2) test units located at `worker/test/` folder. It invokes `make test` below.

### `npm run coverage:node`

Same as `npm run test:node` but it also opens a browser window with JavaScript coverage results.

### `npm run install-deps:node`

Installs NPM dependencies and updates `package-lock.json`.

### `npm run install-clang-tools`

Installs clang tools needed for local development.

## Rust

The only special feature in Rust case is special environment variable `"KEEP_BUILD_ARTIFACTS", that when set to `1` will allow incremental recompilation of changed C++ sources during hacking on mediasoup.
It is not necessary for normal usage of mediasoup as a dependency.

## Makefile

The `worker` folder contains a `Makefile` for the mediasoup-worker C++ subproject. It includes the following tasks:

### `make` or `make mediasoup-worker`

Alias of ``make mediasoup-worker` below.

### `make meson-ninja`

Installs `meson` and `ninja`.

### `make clean`

Cleans built objects and binaries.

### `make clean-build`

Cleans built objects and other artifacts, but keeps `mediasoup-worker` binary in place.

### `make clean-pip`

Cleans `meson` and `ninja` installed in local prefix with pip.

### `make clean-subprojects`

Cleans subprojects downloaded with Meson.

### `make clean-all`

Cleans built objects and binaries, `meson` and `ninja` installed in local prefix with pip and all subprojects downloaded with Meson.

### `make update-wrap-file`

Update the wrap file of a subproject with Meson. Usage example:

```bash
cd worker
make update-wrap-file SUBPROJECT=openssl
```

### `make mediasoup-worker`

Builds the `mediasoup-worker` binary at `worker/out/Release/`.

If the "MEDIASOUP_MAX_CORES" environment variable is set, the build process will use that number of CPU cores. Otherwise it will auto-detect the number of cores in the machine.

"MEDIASOUP_BUILDTYPE" environment variable controls build types, `Release` and `Debug` are presets optimized for those use cases.
Other build types are possible too, but they are not presets and will require "MESON_ARGS" use to customize build configuration.
Check the meaning of useful macros in the `worker/include/Logger.hpp` header file if you want to enable tracing or other debug information.

Binary is built at `worker/out/MEDIASOUP_BUILDTYPE/build`.

In order to instruct the mediasoup Node.js module to use the `Debug` mediasoup-worker binary, an environment variable must be set before running the Node.js application:

```bash
MEDIASOUP_BUILDTYPE=Debug node myapp.js
```

If the "MEDIASOUP_WORKER_BIN" environment variable is set (it must be an absolute file path), mediasoup will use the it as mediasoup-worker binary and **won't** compile the binary:

```bash
MEDIASOUP_WORKER_BIN="/home/xxx/src/foo/mediasoup-worker" node myapp.js
```

### `make libmediasoup-worker`

Builds the `libmediasoup-worker` static library at `worker/out/Release/`.

"MEDIASOUP_MAX_CORES"` and "MEDIASOUP_BUILDTYPE" environment variables from above still apply for static library build.

### `make xcode`

Builds a Xcode project for the mediasoup-worker subproject.

### `make lint`

Validates mediasoup-worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html) and rules in `worker/.clang-format`.

### `make format`

Rewrites mediasoup-worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html).

### `make test`

Builds and runs the `mediasoup-worker-test` binary at `worker/out/Release/` (or at `worker/out/Debug/` if the "MEDIASOUP_BUILDTYPE" environment variable is set to "Debug"), which uses [Catch2](https://github.com/catchorg/Catch2) to run test units located at `worker/test/` folder.


### 'make test-asan'

Run test with Address Sanitizer.

### `make tidy`

Runs [clang-tidy](http://clang.llvm.org/extra/clang-tidy/) and performs C++ code checks following `worker/.clang-tidy` rules.

**Requirements:**

* `make clean` and `make` must have been called first.
* [PyYAML](https://pyyaml.org/) is required.
  - In OSX install it with `brew install libyaml` and `sudo easy_install-X.Y pyyaml`.

"MEDIASOUP_TIDY_CHECKS" environment variable with a comma separated list of checks overrides the checks defined in `.clang-tidy` file.

### `make fuzzer`

Builds the `mediasoup-worker-fuzzer` binary (which uses [libFuzzer](http://llvm.org/docs/LibFuzzer.html)) at `worker/out/Release/` (or at `worker/out/Debug/` if the "MEDIASOUP_BUILDTYPE" environment variable is set to "Debug").

**Requirements:**

* Linux with fuzzer capable clang++.
* `CC` environment variable must point to "clang".
* `CXX` environment variable must point to "clang++".

Read the [Fuzzer](Fuzzer.md) documentation for detailed information.

### `make fuzzer-run-all`

Runs all fuzzer cases.

### `make docker`

Builds a Linux image with fuzzer capable clang++.

**NOTE:** Before running this command, a specific version of Linux clang must be downloaded. To get it, run:

```bash
cd worker
./scripts/get-dep.sh clang-fuzzer
```

### `make docker-run`

Runs a container of the Docker image created with `make docker`. It automatically executes a `bash` session in the `/mediasoup` directory, which is a Docker volume that points to the real `mediasoup` directory.
