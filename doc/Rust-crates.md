# Rust crates

There are 3 crates: `mediasoup`, `mediasoup-sys` and `mediasoup-types`:

- `mediasoup-sys` crate wraps C++ worker into Rust.
- `mediasoup-types` crate defines and exposes mediasoup Rust types.
- `mediasoup` crate uses `mediasoup-sys` and `mediasoup-types` and it exposes nice user API in idiomatic Rust.
- `mediasoup-sys` is the only one that needs updating if changes are purely inside the worker or inside the `mediasoup-sys` crate. You can bump them all, but it is not required.
- If `mediasoup-sys`'s API changes in a breaking way, then its minor version needs to be changed, otherwise patch version needs to be changed. Same for `mediasoup-types` crate.

**Important:** Adding new APIs that `mediasoup` crate has to understand to continue working normally is a breaking change because it'll start crashing/printing errors if unexpected things happen.

## Publishing crates

Each crate is published through an automated flow (mirroring how the NPM package is released): `npm run release:rust <crate> X.Y.Z` bumps the version, commits and pushes, and the `mediasoup-crate-publish.yaml` workflow then publishes the crate to crates.io via OIDC trusted publishing.

Because `mediasoup` depends on `mediasoup-sys` and `mediasoup-types`, when more than one crate needs a new version, **publish the dependencies first** (`mediasoup-types` and/or `mediasoup-sys`) and `mediasoup` last, so each crate's dependencies are already on crates.io when it is published. Each release updates only the released crate's own resolved version in `Cargo.lock` (within that crate's release commit), so releasing a dependency is what refreshes its `Cargo.lock` entry, and the later `mediasoup` release only touches `mediasoup`'s own entry.

You do **not** bump the crate's `[package].version`, edit `rust/CHANGELOG.md`, nor edit the sibling `version` requirements in `rust/Cargo.toml` yourself; `npm run release:rust` does all that. For each crate to publish:

1. Have the crate's code changes merged on the main branch. When publishing the `mediasoup` crate, the changes for the new version must be under the `### NEXT` heading of `rust/CHANGELOG.md`.
2. You do **not** edit the `version` of `[dependencies.mediasoup-sys]` / `[dependencies.mediasoup-types]` in `rust/Cargo.toml` yourself: that requirement is what the _published_ `mediasoup` crate depends on (its `path` is only used for local builds and is dropped when published). Releasing `mediasoup-sys` / `mediasoup-types` automatically bumps that requirement to the just-released version and commits it together with the release, so the `mediasoup` crate always depends on the latest version of its siblings (and the workspace stays buildable after a breaking minor/major bump, where the requirement must keep accepting the sibling's actual version).
3. Make sure `Cargo.lock` is already in sync with the merged code, committing it if an earlier change to third-party dependencies left it stale (run `cargo build`). This is only about pre-existing code changes, **not** the version bump: `npm run release:rust` regenerates `Cargo.lock` for the bumped version itself and commits it within the crate's own release commit. A stale `Cargo.lock` is harmless for crate consumers (they ignore the packaged lock) but leaves the repo out of sync and breaks the `--locked` GitHub Actions workflows, and the release aborts on it.
4. On the main branch, with a clean work tree, run:

```sh
npm run release:rust mediasoup-types X.Y.Z
# and/or
npm run release:rust mediasoup-sys X.Y.Z
# and finally, when a new mediasoup must depend on the version(s) just released
npm run release:rust mediasoup X.Y.Z
```

This runs the checks, bumps the crate version in its `Cargo.toml` (and, for `mediasoup-sys` / `mediasoup-types`, the matching `version` requirement in the `mediasoup` crate's `rust/Cargo.toml`) and in `Cargo.lock`, and then:

- For `mediasoup`: sets the top `### NEXT` heading of `rust/CHANGELOG.md` to `### X.Y.Z`, commits (`release rust-X.Y.Z [no-ci]`), creates the `rust-X.Y.Z` tag and pushes the branch and the tag. The tag triggers `mediasoup-crate-publish.yaml`, which creates the GitHub release from `rust/CHANGELOG.md` and publishes the crate.
- For `mediasoup-sys` / `mediasoup-types`: commits (`<crate> X.Y.Z [crate-publish] [no-ci]`) and pushes the branch (no tag, no GitHub release). The `[crate-publish]` marker is what `mediasoup-crate-publish.yaml` detects on the branch push to publish that crate.

See `npm run release:rust` in [Building.md](Building.md) for more details.
