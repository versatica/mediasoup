# Rust crates

There are 3 crates: `mediasoup`, `mediasoup-sys` and `mediasoup-types`:

- `mediasoup-sys` crate wraps C++ worker into Rust.
- `mediasoup-types` crate defines and exposes mediasoup Rust types.
- `mediasoup` crate uses `mediasoup-sys` and `mediasoup-types` and it exposes nice user API in idiomatic Rust.
- `mediasoup-sys` is the only one that needs updating if changes are purely inside the worker or inside the `mediasoup-sys` crate. You can bump them all, but it is not required.
- If `mediasoup-sys`'s API changes in a breaking way, then its minor version needs to be changed, otherwise patch version needs to be changed. Same for `mediasoup-types` crate.

**Important:** Adding new APIs that `mediasoup` crate has to understand to continue working normally is a breaking change because it'll start crashing/printing errors if unexpected things happen.

## Steps to publish new mediasoup crates

1. Update versions in `worker/Cargo.toml` (for `mediasoup-sys` crate), `rust/types/Cargo.toml` (for `mediasoup-types` crate) and `rust/Cargo.toml` (for `mediasoup` crate). Note that in `rust/Cargo.toml` you may need to update the version of `[dependencies.mediasoup-sys]` and/or `[dependencies.mediasoup-types]` if it also changed.
2. Update `rust/CHANGELOG.md`.
3. Run `cargo build` to reflect changes in `Cargo.lock`.
4. Create PR and have it merged in mediasoup main branch.
5. Upload Git tags (the new one in `rust/CHANGELOG.md`, so the new `mediasoup` crate version):

```sh
git tag -a rust-X.X.X -m rust-X.X.X
git push origin rust-X.X.X
```

6. Publish crates (you need an account and permissions and so on):

```sh
cd rust/types
cargo publish

cd worker
cargo publish

cd rust
cargo publish
```

## Notes

- Depending on the state in `worker` directory you may need to run `invoke clean-all` or `make clean-all` in `worker` directory first.
- `cargo publish` will create the crate package, check if all necessary dependencies are already present on [crates.io](https://crates.io/), will then compile the package (to ensure that you don't publish a broken version) and will upload it to [crates.io](https://crates.io/).
- Never publish from random branches or local state that is not on GitHub. If you have local files modified Cargo will refuse to publish until you commit all the changes.

## Extras

### Check crate without publishing

If you want to do everything except publishing itself, `cargo package` command exists. You can also run `cargo package --dry-run` to avoid package generation or `cargo publish --dry-run`.

### Update required Rust version

Using `rustup` command:

```sh
rustup update
```
