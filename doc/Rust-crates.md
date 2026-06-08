# Rust crates

There are 3 crates: `mediasoup`, `mediasoup-sys` and `mediasoup-types`:

- `mediasoup-sys` crate wraps C++ worker into Rust.
- `mediasoup-types` crate defines and exposes mediasoup Rust types.
- `mediasoup` crate uses `mediasoup-sys` and `mediasoup-types` and it exposes nice user API in idiomatic Rust.
- `mediasoup-sys` is the only one that needs updating if changes are purely inside the worker or inside the `mediasoup-sys` crate. You can bump them all, but it is not required.
- If `mediasoup-sys`'s API changes in a breaking way, then its minor version needs to be changed, otherwise patch version needs to be changed. Same for `mediasoup-types` crate.

**Important:** Adding new APIs that `mediasoup` crate has to understand to continue working normally is a breaking change because it'll start crashing/printing errors if unexpected things happen.

## Steps to publish new mediasoup crates

1. Update versions in `worker/Cargo.toml` (for `mediasoup-sys` crate), `rust/types/Cargo.toml` (for `mediasoup-types` crate) and `rust/Cargo.toml` (for `mediasoup` crate). Note that in `rust/Cargo.toml` you may need to update the version of `[dependencies.mediasoup-sys]` and/or `[dependencies.mediasoup-types]` if they also changed.
2. Update `rust/CHANGELOG.md`.
3. Run `cargo build` to reflect changes in `Cargo.lock` and commit the updated `Cargo.lock`. Do not skip this: a stale `Cargo.lock` is harmless for crate consumers (they ignore the packaged lock) but leaves the repo and CI out of sync.
4. Create PR and have it merged in mediasoup main branch.
5. Upload Git tags (the new one in `rust/CHANGELOG.md`, so the new `mediasoup` crate version):

```sh
git tag -a rust-X.X.X -m rust-X.X.X
git push origin rust-X.X.X
```

6. Publish crates (you need an account and permissions and so on). Always pass `--locked` so Cargo aborts if `Cargo.lock` is out of date instead of silently regenerating it (see note below):

```sh
cd rust/types
cargo publish --locked

cd worker
cargo publish --locked

cd rust
cargo publish --locked
```

## Notes

- Depending on the state in `worker` directory you may need to run `invoke clean-all` or `make clean-all` in `worker` directory first.
- You can have `KEEP_BUILD_ARTIFACTS=1` exported in your shell (handy to speed up regular local builds) and still publish: `mediasoup-sys`'s `build.rs` detects the `cargo publish` / `cargo package` verification step (Cargo builds the crate inside `target/package/`) and always cleans the Meson subprojects it extracts into the source tree, regardless of `KEEP_BUILD_ARTIFACTS`. This avoids the "Source directory was modified by build.rs" error.
- `cargo publish` will create the crate package, check if all necessary dependencies are already present on [crates.io](https://crates.io/), will then compile the package (to ensure that you don't publish a broken version) and will upload it to [crates.io](https://crates.io/).
- Never publish from random branches or local state that is not on GitHub. If you have local files modified Cargo will refuse to publish until you commit all the changes.
- Use `cargo publish --locked`. The dirty-repo check runs _before_ the verification build, but the verification build is exactly when Cargo regenerates `Cargo.lock`, so a stale lock slips past the dirty check and gets silently updated. `--locked` makes `cargo publish` fail up front if `Cargo.lock` needs updating, forcing step 3 to have been done and committed first.

## Extras

### Check crate without publishing

- If you want to do everything except the upload itself, run `cargo publish --dry-run`, which creates the package and compiles it for verification but does not upload anything.
- To dry-run all crates at once (recommended before bumping versions), pass them as a single group so Cargo resolves the dependencies among them against the locally packaged copies instead of [crates.io](https://crates.io/). This works even if the new versions are not published yet:

```sh
cargo publish --dry-run -p mediasoup-types -p mediasoup-sys -p mediasoup
```

- To only build the package tarball without uploading, use `cargo package`.
- To just list the files that would be included, use `cargo package --list`.

### Update required Rust version

Using `rustup` command:

```sh
rustup update
```
