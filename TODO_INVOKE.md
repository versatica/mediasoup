# TODO pip invoke

- I've renamed `MEDIASOUP_OUT_DIR` to `OUT_DIR`, so check if set/used somewhere else.

- `build.rs`: Remvoe the `MEDIASOUP_OUT_DIR` env stuff.

- Remove `getmake.py` and related stuff.

- When running meson commands we are loosing colored output, perhaps some invoke `context.run()` option is needed. Reported here: https://github.com/pyinvoke/invoke/discussions/976

- Every time I run `invoke test` or `make test` the whole worker is built from scratch :( This may happen because we run `flatc` task anyway in `tasks.py` (maybe a TODO), but why when using `make test`???
