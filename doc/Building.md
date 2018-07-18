# Building

This document is intended for **mediasoup** developers.


## Makefile

The root folder of the project contains a `Makefile` to build the mediasoup worker subproject (under the `worker/` folder).

### `make`

The default task runs the `Release` task unless the environment `MEDIASOUP_BUILDTYPE` is set to `Debug` (if so it runs the `Debug` task).

### `make Release`

Builds the production ready mediasoup worker binary at `worker/out/Release/`. This is the binary used in production when installing the **mediasoup** NPM module with `npm install mediasoup`.

### `make Debug`

Builds a more verbose and non optimized mediasoup worker binary at `worker/out/Debug/` with some C flags enabled (such as `-O0`) and some macros defined (such as `DEBUG`, `MS_LOG_TRACE` and `MS_LOG_FILE_LINE`).

Check the meaning of these macros in the [Logger.hpp](worker/include/Logger.hpp) header file.

In order to instruct the **mediasoup** Node.js module to use the `Debug` mediasoup worker binary, an environment variable must be set before running the Node.js application:

```bash
$ MEDIASOUP_BUILDTYPE=Debug node myapp.js
```

### `make test`

Runs the `test-Release` task unless the environment `MEDIASOUP_BUILDTYPE` is set to `Debug` (if so it runs the `test-Debug` task).

### `make test-Release`

Builds the `mediasoup-worker-test` test unit binary at `worker/out/Release/`.

### `make test-Debug`

Builds the `mediasoup-worker-test` test unit binary in `Debug` mode at `worker/out/Debug/`.

### `make xcode`

Builds a Xcode project for the mediasoup worker subproject.

### `make clean`

Cleans objects and binaries related to the mediasoup worker.

### `make clean-all`

Cleans all the objects and binaries, including those generated for library dependencies (such as libuv, openssl and libsrtp).


## gulpfile.js

**mediasoup** comes with a `gulpfile.js` file to enable [gulp](https://www.npmjs.com/package/gulp) tasks.

In order to tun these tasks, `gulp-cli` (version >= 1.2.2) must be globally installed:

```bash
$ npm install -g gulp-cli
```

### `gulp`

The default task runs the `gulp:lint` and `gulp:test` tasks.

### `gulp rtpcapabilities`

Reads **mediasoup** [supported RTP capabilities](https://github.com/versatica/mediasoup/blob/master/lib/supportedRtpCapabilities.js) and inserts them into the worker C++ code. After that, `make Release` and/or `make Debug` must be called.

### `gulp lint`

Runs both the `lint:node` and `lint:worker` gulp tasks.

### `gulp lint:node`

Validates the Node.js JavaScript code/syntax.

### `gulp lint:worker`

Validates the worker C++ code/syntax using [clang-format](https://clang.llvm.org/docs/ClangFormat.html) following `worker/.clang-format` rules.

### `gulp format`

Runs the `format:worker` gulp task.

### `gulp format:worker`

Rewrites worker source and include files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html).

### `gulp tidy`

Runs the `tidy:worker` gulp task.

### `gulp tidy:worker`

Performs C++ code check using [clang-tidy](http://clang.llvm.org/extra/clang-tidy/) following following `worker/.clang-tidy` rules.

clang-tidy uses a [JSON compilation database](http://clang.llvm.org/docs/JSONCompilationDatabase.html) to get the information on how a single compilation unit is processed.

In order to generate this file we use [Bear](https://github.com/rizsotto/Bear). This works on linux only.

Once installed, compile mediasoup as follows:

```bash
$ bear make
```
An output file `compile_commands.json` is generated which must be copied to the [worker](worker/) directory after making the following changes:

Replace the references of the current directory by the keywork 'PATH':

```bash
$  sed -i "s|$PWD|PATH|g" compile_commands.json
```

Edit the file and remove the entry related to Utils/IP.cpp compilation unit, which is automatically created and does not follow the clang-tidy rules.

*NOTE:* It just works on Linux and OSX.

### `gulp test`

Runs both the `test:node` and `test:worker` gulp tasks.

### `gulp test:node`

Runs the Node.js [test units](test/). Before it, it invokes the `make` command.

In order to run the JavaScript test units with the mediasoup worker in `Debug` mode the `MEDIASOUP_BUILDTYPE` environment variable must be set to `Debug`:

```bash
$ MEDIASOUP_BUILDTYPE=Debug gulp test:node
```

### `gulp test:worker`

Runs the mediasoup worker [test units](worker/test/). Before it, it invokes the `make test` command.

In order to run the worker test units with the mediasoup worker in `Debug` mode the `MEDIASOUP_BUILDTYPE` environment variable must be set to `Debug`:

```bash
$ MEDIASOUP_BUILDTYPE=Debug gulp test:worker
```
