# TODO pip invoke

### pip `invoke` package is required

This solution requires having the pip invoke package installed, however I think
we could just include it in the mediasoup repo in the worker/pip-invoke folder
and make npm-scripts.mjs invoke its executable in worker/pip-invoke/bin/invoke
(same for Rust). However that's hard since we also need to later override THONPATH
env for pip invoke module to be found. So...
Perhaps it's easier if we run worker/scripts/get-pip-invoke.sh during the
installation. That script installs pip invoke package in default pip location.
Or we can set PYTHONPATH env within npm-scripts.mjs (and Rust) and also in the
Makefile, so we don't remove it and instead make it behave as a proxy to invoke.
Let's see.

In summary:
- `build.rs` needs to call `invoke` instead of `make`. Should we mandate `pip install invoke` when using mediasoup-rust?
- Same for `npm-scripts.mjs` which now should call `invoke` instead of `make`.
- Check if current `worker/scripts/get-pip-invoke.sh` script makes sense.


### Other TODO items

- `MESON_ARGS` stuff. How to replicate this? What is this `subst` doing exactly?
  ```Makefile
  ifeq ($(OS),Windows_NT)
  ifeq ($(MESON_ARGS),"")
  	MESON_ARGS = $(subst $\",,"--vsenv")
  endif
  endif
  ```

- `build.rs`: Remvoe the `MEDIASOUP_OUT_DIR` env stuff.

- Remove `getmake.py` and related stuff.

- When running meson commands we are loosing colored output, perhaps some invoke `context.run()` option is needed. Reported here: https://github.com/pyinvoke/invoke/discussions/976. Solved using `pty=True` in `ctx.run()` but I need to check the implications. So I'm adding `pty=True` everywehere. Let's see.

- Do we still need this in `build.rs`?
  ```rust
  // Force forward slashes on Windows too so that is plays well with our dumb `Makefile`.
  let mediasoup_out_dir = format!("{}/out", out_dir.replace('\\', "/"));
  ```

- "Code QL Analyze (cpp)" CI task fails because "Command failed: invoke -r worker" (as expected): https://github.com/versatica/mediasoup/actions/runs/6973590508/job/18977838894?pr=1239

- Once we have `invoke` working, let's remove the no longer used `cpu_cores.sh` script.

- And of course, remove `Makefile`.
