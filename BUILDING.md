# Building

This document is intended for **mediasoup** developers.


## Makefile

The root folder of the project contains a `Makefile` to build the mediasoup worker subproject (under the `worker/` folder).

### `make Release`

Builds the production ready mediasoup worker binary at `worker/out/Release/`. This is the binary used in production when installing the **mediasoup** NPM module with `npm install mediasoup`.

### `make Debug`

Builds a more verbose mediasoup and less optimized worker binary at `worker/out/Debug/` with some C flags enabled (such as `-g` and `-O0`) and some macros defined (such as `DEBUG`, `MS_LOG_FILE_LINE` and `MS_LOG_TRACE`).

Check the meaning of these macros in the [Logger.h](worker/include/Logger.h) header file.

In order to instruct the **mediasoup** Node.js module to use the `Debug` mediasoup worker binary, an environment variable must be set before running the Node.js application or the internal test units:

```bash
$ MEDIASOUP_BUILDTYPE=Debug node myapp.js
```

Or, in order to run the internal test units with the `Debug` mediasoup worker:

```bash
$ MEDIASOUP_BUILDTYPE=Debug gulp test-debug
```

### `make xcode`

Builds a Xcode project for the mediasoup worker subproject.

### `make clean`

Cleans objects and binaries related to the mediasoup worker.

### `make clean-all`

Clean all the objects and binaries, including those generated for library dependencies (such as libuv, openssl and libsrtp).

### `make`

The default task runs the `Release` task.


## gulpfile.js

**mediasoup** comes with a `gulpfile.js` file to enable [gulp](https://www.npmjs.com/package/gulp) tasks.

In order to tun these tasks, `gulp-cli` must be globally installed:

```bash
$ npm install -g gulp-cli
```

### `gulp lint`

Validates the JavaScript code.

### `gulp capabilities`

Reads **mediasoup** [media capabilities](https://github.com/ibc/mediasoup/blob/master/data/supportedCapabilities.js) and inserts them into the worker C++ code. After that, `make Release` or `make Debug` must be called.

### `gulp test`

Run the internal [test units](test/).

### `gulp test-debug`

Run the internal [test units](test/) in a more verbose fashion. Note however that running this task does not use the `Debug` mediasoup worker binary. If you wish to use it, set the `MEDIASOUP_BUILDTYPE` environment variable as explained above.

### `gulp`

The default task runs the `link` task.
