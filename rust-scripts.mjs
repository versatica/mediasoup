import * as process from 'node:process';
import * as fs from 'node:fs';
import { execSync } from 'node:child_process';
import fetch from 'node-fetch';
import pkg from './package.json' with { type: 'json' };

const GH_OWNER = 'versatica';
const GH_REPO = 'mediasoup';
// Main Git branch is 'v' concatenated with the major SEMVER number of the
// "version" field in package.json.
const MAIN_BRANCH = `v${pkg.version.split('.')[0]}`;
// The three publishable mediasoup Rust crates. For each one: `manifest` is the
// Cargo.toml that holds its `[package].version`, and `dependents` lists the
// workspace manifests that declare it as a dependency (whose `version`
// requirement is bumped to the released version too). The `mediasoup` crate is
// special: releasing it creates a `rust-X.Y.Z` Git tag and a GitHub release
// built from its CHANGELOG; the other two are published without a tag or a
// GitHub release (the `mediasoup-crate-publish.yaml` workflow detects them from
// the commit message).
const CRATES = [
	{ name: 'mediasoup', manifest: 'rust/Cargo.toml' },
	{
		name: 'mediasoup-sys',
		manifest: 'worker/Cargo.toml',
		dependents: ['rust/Cargo.toml'],
	},
	{
		name: 'mediasoup-types',
		manifest: 'rust/types/Cargo.toml',
		dependents: ['rust/Cargo.toml'],
	},
];
// The `mediasoup` crate, whose rust/CHANGELOG.md drives the GitHub release body.
const MEDIASOUP_CRATE = CRATES[0];
const MEDIASOUP_CRATE_CHANGELOG = 'rust/CHANGELOG.md';

const task = process.argv[2];
const taskArgs = process.argv.slice(3).join(' ');

void run();

async function run() {
	logInfo(taskArgs ? `[args:"${taskArgs}"]` : '');

	switch (task) {
		case 'release:check': {
			await checkRelease();

			break;
		}

		case 'release': {
			await release({ args: taskArgs });

			break;
		}

		default: {
			logError('unknown task');

			exitWithError();
		}
	}
}

function lint() {
	logInfo('lint()');

	executeCmd('cargo fmt --all -- --check');
	executeCmd('cargo clippy --all-targets -- -D warnings');
}

function test() {
	logInfo('test()');

	executeCmd('cargo test --verbose');
	executeCmd('cargo test --release --verbose');
}

function doc() {
	logInfo('doc()');

	// Fail on broken/private intra-doc links, same as when building the docs for
	// docs.rs.
	process.env.DOCS_RS = '1';
	process.env.RUSTDOCFLAGS =
		'-D rustdoc::broken-intra-doc-links -D rustdoc::private_intra_doc_links';

	executeCmd('cargo doc --locked --all --no-deps --lib');
}

/**
 * Validates packaging of all mediasoup Rust crates without uploading anything.
 * They are passed as a single group so cargo resolves the dependencies among
 * them against the locally packaged copies instead of crates.io (works even if
 * the new versions are not published yet).
 */
function publishDryRun() {
	logInfo('publishDryRun()');

	executeCmd(
		'cargo publish --dry-run -p mediasoup-types -p mediasoup-sys -p mediasoup'
	);
}

async function checkRelease(crate = MEDIASOUP_CRATE) {
	logInfo(`checkRelease() [crate:${crate.name}]`);

	const { version } = readCrate(crate.manifest);

	// rust/CHANGELOG.md tracks only the `mediasoup` crate, so the CHANGELOG entry
	// is verified (and grabbed) only when releasing that crate, before the slow
	// build steps.
	let versionChanges;

	if (crate.name === 'mediasoup') {
		try {
			versionChanges = await getVersionChanges(version);
		} catch (error) {
			logError(`checkRelease() | ${error.message}`);

			exitWithError();
		}
	}

	// Ensure Cargo.lock is in sync before running any cargo command that could
	// silently regenerate it.
	checkCargoLock();

	lint();
	test();
	doc();

	// Validate packaging only when the crate version is not yet on crates.io (i.e.
	// it has been bumped and is about to be published). Otherwise Cargo resolves
	// the dependencies among the three crates against the already-published copies
	// on crates.io and any schema/API change made since the last release fails
	// verification spuriously even though nothing is being published (same gating
	// as in `mediasoup-rust.yaml`).
	let published;

	try {
		published = await isCratePublished(crate.name, version);
	} catch (error) {
		logError(`checkRelease() | ${error.message}`);

		exitWithError();
	}

	if (published) {
		logInfo(
			`checkRelease() | ${crate.name} ${version} is already published on crates.io, skipping publish dry-run`
		);
	} else {
		publishDryRun();
	}

	return { versionChanges };
}

async function release({ args = '' } = {}) {
	logInfo('release()');

	const [name, version] = args.trim().split(/\s+/);

	const crate = CRATES.find(entry => entry.name === name);

	if (!crate) {
		logError(
			`release() | first argument must be a crate name (one of: ${CRATES.map(
				entry => `'${entry.name}'`
			).join(', ')}), but got '${name}'`
		);

		exitWithError();
	}

	if (!version || !/^\d+\.\d+\.\d+$/.test(version)) {
		logError(
			`release() | a SEMVER 'x.y.z' version is required as second argument, but got '${version}'`
		);

		exitWithError();
	}

	// Must be on the main branch.
	const branch = execSync('git rev-parse --abbrev-ref HEAD', {
		encoding: 'utf-8',
	}).trim();

	if (branch !== MAIN_BRANCH) {
		logError(
			`release() | must be on '${MAIN_BRANCH}' branch, but it is on '${branch}' branch`
		);

		exitWithError();
	}

	// Clean working tree required before bumping the version.
	checkGitClean();

	// Lint, test, doc, publish dry-run, and (for the `mediasoup` crate) verify the
	// CHANGELOG entry. Runs before the bump (the checked version is the previous
	// one still in the manifest, which is harmless).
	await checkRelease(crate);

	// Bump the crate version in its Cargo.toml and reflect it in the (workspace)
	// root Cargo.lock.
	bumpCargoVersion(crate.manifest, version);

	// Keep every workspace manifest that depends on this crate pointing at the
	// just-released version (the `mediasoup` crate's `version` requirement on
	// `mediasoup-sys` / `mediasoup-types` in rust/Cargo.toml). These are
	// `path` + `version` dependencies, so the requirement must keep accepting the
	// sibling's actual version (otherwise a breaking bump would fail the
	// `cargo metadata` in syncCargoLock()); bumping it here also makes the next
	// `mediasoup` release depend on the new version, and it is committed together
	// with this release commit.
	for (const dependent of crate.dependents ?? []) {
		updateDependencyVersion(dependent, crate.name, version);
	}

	syncCargoLock();

	if (crate.name === 'mediasoup') {
		// The `mediasoup` crate also bumps rust/CHANGELOG.md and is released via a
		// `rust-X.Y.Z` Git tag, whose push triggers `mediasoup-crate-publish.yaml`
		// to create the GitHub release and publish the crate to crates.io. On its
		// success `mediasoup-website-update.yaml updates the website.
		await updateChangelog(version);

		const tag = `rust-${version}`;

		// Commit the bump, tag it, and push both.
		//
		// The commit message carries a "[no-ci]" marker so the regular branch CI
		// workflows (node, worker, rust, fuzzer, codeql) skip this commit: it only
		// bumps version/CHANGELOG (no code change) and its parent already passed CI,
		// and the release is driven by the tag-triggered workflows instead.
		//
		// NOTE: "[no-ci]" (with a hyphen) is a custom marker, NOT GitHub's native
		// "[skip ci]"/"[no ci]" (which would also skip mediasoup-crate-publish,
		// since the tag push shares this same commit).
		executeCmd(`git commit -am 'release ${tag} [no-ci]'`);
		executeCmd(`git tag -a ${tag} -m '${tag}'`);
		executeCmd(`git push origin ${MAIN_BRANCH}`);
		executeCmd(`git push origin '${tag}'`);
	} else {
		// The `mediasoup-sys` / `mediasoup-types` crates are released without a Git
		// tag or GitHub release. The commit message
		// `<crate> <version> [crate-publish] [no-ci]` is the signal that
		// `mediasoup-crate-publish.yaml` parses on the branch push: the
		// "[crate-publish]" marker opts the commit into the workflow, and the crate
		// name and version are read from the start of the message.
		//
		// "[no-ci]" keeps the regular branch CI workflows (node, worker, rust,
		// fuzzer, codeql) from running on this version-only commit.
		executeCmd(
			`git commit -am '${crate.name} ${version} [crate-publish] [no-ci]'`
		);
		executeCmd(`git push origin ${MAIN_BRANCH}`);
	}
}

function checkGitClean() {
	logInfo('checkGitClean()');

	const status = execSync('git status --porcelain', {
		encoding: 'utf-8',
		stdio: ['ignore', 'pipe', 'ignore'],
	});

	if (status.trim()) {
		logError(
			'checkGitClean() | Git working tree is not clean, commit or stash your changes first'
		);

		exitWithError();
	}
}

function checkCargoLock() {
	logInfo('checkCargoLock()');

	// `--locked` makes cargo fail if Cargo.lock is out of date instead of
	// regenerating it. We resolve metadata (no build) just to assert the lock is
	// in sync, otherwise run `cargo build` and commit Cargo.lock first.
	try {
		execSync('cargo metadata --locked --format-version 1', {
			stdio: ['ignore', 'ignore', process.stderr],
		});
	} catch (error) {
		logError(
			'checkCargoLock() | Cargo.lock is out of date, run `cargo build` and commit it'
		);

		exitWithError();
	}
}

function syncCargoLock() {
	logInfo('syncCargoLock()');

	// Reflect the just-bumped crate version in the (workspace) root Cargo.lock.
	// `cargo metadata` resolves the workspace and rewrites Cargo.lock, changing
	// only the bumped member version while keeping every other pinned dependency
	// intact (checkCargoLock() asserted the lock was in sync beforehand).
	try {
		execSync('cargo metadata --format-version 1', {
			stdio: ['ignore', 'ignore', process.stderr],
		});
	} catch (error) {
		logError(`syncCargoLock() failed, exiting: ${error}`);

		exitWithError();
	}
}

async function getVersionChanges(version) {
	logInfo(`getVersionChanges() [version:${version}]`);

	// NOTE: Load dep on demand since it's a devDependency.
	const marked = await import('marked');

	const changelog = fs.readFileSync(MEDIASOUP_CRATE_CHANGELOG, {
		encoding: 'utf-8',
	});
	const entries = marked.lexer(changelog);

	for (let idx = 0; idx < entries.length; ++idx) {
		const entry = entries[idx];

		if (entry.type === 'heading' && entry.text === version) {
			// Collect every token after the matching heading until the next heading.
			// NOTE: We cannot just use `entries[idx + 1].raw` because `marked`
			// inserts a `space` token between the heading and its content.
			let changes = '';

			for (let next = idx + 1; next < entries.length; ++next) {
				if (entries[next].type === 'heading') {
					break;
				}

				changes += entries[next].raw;
			}

			changes = changes.trim();

			if (changes) {
				return changes;
			}

			break;
		}
	}

	// This should not happen (unless author forgot to update the CHANGELOG).
	throw new Error(
		`no entry found in ${MEDIASOUP_CRATE_CHANGELOG} for version '${version}'`
	);
}

async function updateChangelog(version) {
	logInfo(`updateChangelog() [version:${version}]`);

	// NOTE: Load dep on demand since it's a devDependency.
	const marked = await import('marked');

	const changelog = fs.readFileSync(MEDIASOUP_CRATE_CHANGELOG, {
		encoding: 'utf-8',
	});
	const tokens = marked.lexer(changelog);

	// Locate the top "### NEXT" heading.
	const nextHeading = tokens.find(
		token =>
			token.type === 'heading' && token.depth === 3 && token.text === 'NEXT'
	);

	if (!nextHeading) {
		throw new Error(
			`no '### NEXT' heading found in ${MEDIASOUP_CRATE_CHANGELOG}`
		);
	}

	// Insert "### <version>" right below "### NEXT" (keeping the empty "### NEXT"
	// for future unreleased changes), preserving the heading's trailing newlines.
	const updatedChangelog = changelog.replace(
		nextHeading.raw,
		`### NEXT\n\n### ${version}${nextHeading.raw.slice('### NEXT'.length)}`
	);

	fs.writeFileSync(MEDIASOUP_CRATE_CHANGELOG, updatedChangelog);
}

/**
 * Rewrites the `version` key of the `[package]` section of the given Cargo.toml,
 * leaving the `version` keys under `[dependencies.*]` sections untouched.
 */
function bumpCargoVersion(manifestPath, version) {
	logInfo(`bumpCargoVersion() [manifest:${manifestPath}, version:${version}]`);

	const content = fs.readFileSync(manifestPath, { encoding: 'utf-8' });
	const lines = content.split('\n');

	let inPackageSection = false;
	let bumped = false;

	for (let idx = 0; idx < lines.length; ++idx) {
		const trimmed = lines[idx].trim();

		if (trimmed.startsWith('[')) {
			inPackageSection = trimmed === '[package]';

			continue;
		}

		if (inPackageSection && /^version\s*=\s*"[^"]+"/.test(trimmed)) {
			lines[idx] = lines[idx].replace(
				/version\s*=\s*"[^"]+"/,
				`version = "${version}"`
			);
			bumped = true;

			break;
		}
	}

	if (!bumped) {
		logError(
			`bumpCargoVersion() | no 'version' key found in [package] section of '${manifestPath}'`
		);

		exitWithError();
	}

	fs.writeFileSync(manifestPath, lines.join('\n'));
}

/**
 * Rewrites the `version` key of the `[dependencies.<depName>]` table of the
 * given Cargo.toml (the table form used across the workspace, e.g.
 * `[dependencies.mediasoup-sys]`), leaving its `path` and any other keys
 * untouched. Used to keep the `mediasoup` crate's requirement on a sibling crate
 * in sync when that sibling is released.
 */
function updateDependencyVersion(manifestPath, depName, version) {
	logInfo(
		`updateDependencyVersion() [manifest:${manifestPath}, dep:${depName}, version:${version}]`
	);

	const content = fs.readFileSync(manifestPath, { encoding: 'utf-8' });
	const lines = content.split('\n');
	const sectionHeader = `[dependencies.${depName}]`;

	let inDepSection = false;
	let bumped = false;

	for (let idx = 0; idx < lines.length; ++idx) {
		const trimmed = lines[idx].trim();

		if (trimmed.startsWith('[')) {
			inDepSection = trimmed === sectionHeader;

			continue;
		}

		if (inDepSection && /^version\s*=\s*"[^"]+"/.test(trimmed)) {
			lines[idx] = lines[idx].replace(
				/version\s*=\s*"[^"]+"/,
				`version = "${version}"`
			);
			bumped = true;

			break;
		}
	}

	if (!bumped) {
		logError(
			`updateDependencyVersion() | no 'version' key found in '${sectionHeader}' section of '${manifestPath}'`
		);

		exitWithError();
	}

	fs.writeFileSync(manifestPath, lines.join('\n'));
}

function readCrate(manifestPath) {
	const content = fs.readFileSync(manifestPath, { encoding: 'utf-8' });

	return parseManifest(content);
}

/**
 * Extracts `name` and `version` from the `[package]` section of a Cargo.toml,
 * ignoring the `version` keys under `[dependencies.*]` sections.
 */
function parseManifest(content) {
	let inPackageSection = false;
	let name;
	let version;

	for (const line of content.split('\n')) {
		const trimmed = line.trim();

		// A new section starts. We only care about `[package]`.
		if (trimmed.startsWith('[')) {
			inPackageSection = trimmed === '[package]';

			continue;
		}

		if (!inPackageSection) {
			continue;
		}

		const nameMatch = trimmed.match(/^name\s*=\s*"([^"]+)"/);

		if (nameMatch) {
			name = nameMatch[1];

			continue;
		}

		const versionMatch = trimmed.match(/^version\s*=\s*"([^"]+)"/);

		if (versionMatch) {
			version = versionMatch[1];
		}
	}

	if (!name || !version) {
		throw new Error(
			"failed to parse 'name'/'version' from Cargo.toml [package] section"
		);
	}

	return { name, version };
}

/**
 * Tells whether the given crate version is already published on crates.io by
 * querying its API. Throws if crates.io cannot be reached or replies with an
 * unexpected status (so we never publish or skip based on a wrong guess).
 */
async function isCratePublished(name, version) {
	const url = `https://crates.io/api/v1/crates/${name}/${version}`;

	let res;

	try {
		res = await fetch(url, {
			// NOTE: crates.io requires a meaningful User-Agent.
			headers: {
				'User-Agent': `${GH_OWNER}/${GH_REPO} (https://github.com/${GH_OWNER}/${GH_REPO})`,
			},
		});
	} catch (error) {
		// eslint-disable-next-line preserve-caught-error
		throw new Error(
			`failed to reach crates.io for '${name}@${version}': ${error}`
		);
	}

	// 200 means the version exists, 404 means it does not.
	if (res.status === 200) {
		return true;
	} else if (res.status === 404) {
		return false;
	}

	throw new Error(
		`unexpected crates.io response for '${name}@${version}': ${res.status} ${res.statusText}`
	);
}

function executeCmd(command, { cwd } = {}) {
	logInfo(`executeCmd(): ${command}${cwd ? ` [cwd:${cwd}]` : ''}`);

	try {
		execSync(command, {
			cwd,
			stdio: ['ignore', process.stdout, process.stderr],
		});
	} catch (error) {
		logError(`executeCmd() failed, exiting: ${error}`);

		exitWithError();
	}
}

// eslint-disable-next-line no-unused-vars
function executeInteractiveCmd(command, { cwd } = {}) {
	logInfo(`executeInteractiveCmd(): ${command}${cwd ? ` [cwd:${cwd}]` : ''}`);

	try {
		execSync(command, { cwd, stdio: 'inherit', env: process.env });
	} catch (error) {
		logError(`executeInteractiveCmd() failed, exiting: ${error}`);

		exitWithError();
	}
}

function logInfo(...args) {
	// eslint-disable-next-line no-console
	console.log(`rust-scripts.mjs \x1b[36m[INFO] [${task}]\x1b[0m`, ...args);
}

// eslint-disable-next-line no-unused-vars
function logWarn(...args) {
	// eslint-disable-next-line no-console
	console.warn(`rust-scripts.mjs \x1b[33m[WARN] [${task}]\x1b[0m`, ...args);
}

function logError(...args) {
	// eslint-disable-next-line no-console
	console.error(`rust-scripts.mjs \x1b[31m[ERROR] [${task}]\x1b[0m`, ...args);
}

function exitWithError() {
	process.exit(1);
}
