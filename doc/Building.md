# Building

This document is intended for mediasoup developers.


## NPM scripts

The `package.json` file in the main folder includes the following scripts:


### `npm run typescript:build`

Compiles mediasoup TypeScript code (`lib` folder) JavaScript and places it into the `lib` directory.


### `npm run typescript:watch`

Compiles mediasoup TypeScript code (`lib` folder) JavaScript, places it into the `lib` directory an watches for changes in the TypeScript files.

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


## Makefile

The `worker` folder contains a `Makefile` for the mediasoup-worker C++ subproject. It includes the following tasks:


### `make`

Builds the `mediasoup-worker` binary at `worker/out/Release/`.

If the "MEDIASOUP_MAX_CORES" environment variable is set, the build process will use that number of CPU cores. Otherwise it will auto-detect the number of cores in the machine.

If the "MEDIASOUP_BUILDTYPE" environment variable is set to "Debug", the binary is built at `worker/out/Debug/` with some C/C++ flags enabled (such as `-O0`) and some macros defined (such as `DEBUG`, `MS_LOG_TRACE` and `MS_LOG_FILE_LINE`). Check the meaning of these macros in the `worker/include/Logger.hpp` header file.

In order to instruct the mediasoup Node.js module to use the `Debug` mediasoup-worker binary, an environment variable must be set before running the Node.js application:

```bash
$ MEDIASOUP_BUILDTYPE=Debug node myapp.js
```

If the "MEDIASOUP_WORKER_BIN" environment variable is set, mediasoup will use the it as mediasoup-worker binary:

```bash
$ MEDIASOUP_WORKER_BIN="/home/xxx/src/foo/mediasoup-worker" node myapp.js
```


### `make clean`

Cleans built objects and binaries.


### `make clean-all`

Cleans all objects and binaries, including those generated for library dependencies (such as libuv, openssl, libsrtp, etc).


### `make xcode`

Builds a Xcode project for the mediasoup-worker subproject.


### `make lint`

Validates mediasoup-worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html) and rules in `worker/.clang-format`.


### `make format`

Rewrites mediasoup-worker C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html).


### `make test`

Builds and runs the `mediasoup-worker-test` binary at `worker/out/Release/` (or at `worker/out/Debug/` if the "MEDIASOUP_BUILDTYPE" environment variable is set to "Debug"), which uses [Catch2](https://github.com/catchorg/Catch2) to run test units located at `worker/test/` folder.


### `make bear`

Generates the `worker/compile_commands_template.json` file which is a ["Clang compilation database"](https://clang.llvm.org/docs/JSONCompilationDatabase.html).

**Requirements:**

* [Bear](https://github.com/rizsotto/Bear) is required.
  - Install it in Debian/Ubuntu via `apt install bear` and in OSX via `brew install bear`.
  - For now, Bear version must be 2.1.X.
* Before running `make bear` you must have mediasoup C/C++ dependencies already compiled. To be sure, run `make clean-all && make` before running `make bear`.


### `make tidy`

Runs [clang-tidy](http://clang.llvm.org/extra/clang-tidy/) and performs C++ code checks following `worker/.clang-tidy` rules.

**Requirements:**

* `make clean-all`, then `make` and then `make bear` must have been called first.
* [PyYAML](https://pyyaml.org/) is required.
  - In OSX install it with `brew install libyaml` and `sudo easy_install-X.Y pyyaml`.


### `make fuzzer`

Builds the `mediasoup-worker-fuzzer` binary (which uses [libFuzzer](http://llvm.org/docs/LibFuzzer.html)) at `worker/out/Release/` (or at `worker/out/Debug/` if the "MEDIASOUP_BUILDTYPE" environment variable is set to "Debug").

**Requirements:**

* Linux with fuzzer capable clang++.
* `CC` environment variable must point to "clang".
* `CXX` environment variable must point to "clang++".

Read the [Fuzzer](Fuzzer.md) documentation for detailed information.


### `make fuzzer-run-all`

Runs all fuzzer cases.


### `make docker-build`

Builds a Linux image with fuzzer capable clang++.

**NOTE:** Before running this command, a specific version of Linux clang must be downloaded. To get it, run:

```bash
$ cd worker
$ ./scripts/get-dep.sh clang-fuzzer
```


### `make docker-run`

Runs a container of the Docker image created with `make docker-build`. It automatically executes a `bash` session in the `/mediasoup` directory, which is a Docker volume that points to the real `mediasoup` directory.
