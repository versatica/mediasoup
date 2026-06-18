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

// The three Rust crates with their Cargo.toml manifest and directory (relative
// to repo root). Listed in the order in which they must be published to
// crates.io (dependencies first).
const CRATES = [
	// `mediasoup-types` crate.
	{ manifest: 'rust/types/Cargo.toml', dir: 'rust/types' },
	// `mediasoup-sys` crate.
	{ manifest: 'worker/Cargo.toml', dir: 'worker' },
	// `mediasoup` crate (depends on the two above).
	{ manifest: 'rust/Cargo.toml', dir: 'rust' },
];

// The `mediasoup` crate (rust/Cargo.toml) drives both the Git tag (rust-X.X.X)
// and the rust/CHANGELOG.md entry that the GitHub release is built from.
const MEDIASOUP_MANIFEST = 'rust/Cargo.toml';
const RUST_CHANGELOG = 'rust/CHANGELOG.md';

const task = process.argv[2];

void run();

async function run() {
	logInfo('');

	switch (task) {
		case 'release:check': {
			await checkRelease();

			break;
		}

		case 'release': {
			await release();

			break;
		}

		default: {
			logError('unknown task');

			exitWithError();
		}
	}
}

/**
 * Validates that a Rust release can be made: checks which crate versions are not
 * yet on crates.io, that rust/CHANGELOG.md has a matching entry (if the
 * `mediasoup` crate itself is being published) and that the crates build/package.
 * Returns the data needed to perform the release, or null if there is nothing to
 * release.
 */
async function checkRelease() {
	logInfo('checkRelease()');

	// Read the current version of every Rust crate.
	const crates = CRATES.map(crate => ({
		...crate,
		...readCrate(crate.manifest),
	}));

	for (const crate of crates) {
		logInfo(
			`checkRelease() | crate '${crate.name}' version is ${crate.version}`
		);
	}

	// The `mediasoup` crate version is the one used for the Git tag and the
	// CHANGELOG entry.
	const mediasoupVersion = crates.find(
		crate => crate.manifest === MEDIASOUP_MANIFEST
	).version;
	const tag = `rust-${mediasoupVersion}`;

	// Ensure Cargo.lock is in sync before running any cargo command that could
	// silently regenerate it.
	checkCargoLock();

	// Run the cargo checks always, even if there ends up being nothing to publish.
	lintRust();
	testRust();
	docRust();

	// Detect which crates have a version not yet published on crates.io.
	let cratesToPublish;

	try {
		cratesToPublish = await getUnpublishedCrates(crates);
	} catch (error) {
		logError(error.message);

		exitWithError();
	}

	if (cratesToPublish.length === 0) {
		logInfo(
			'checkRelease() | all Rust crate versions are already published, nothing to release'
		);

		return null;
	}

	logInfo(
		`checkRelease() | crates to publish: ${cratesToPublish
			.map(crate => `${crate.name} (${crate.version})`)
			.join(', ')}`
	);

	// The Git tag and GitHub release are created only when the `mediasoup` crate
	// itself is being published (they are keyed on its version).
	const releaseMediasoup = cratesToPublish.some(
		crate => crate.manifest === MEDIASOUP_MANIFEST
	);

	let versionChanges;

	if (releaseMediasoup) {
		// Verify that rust/CHANGELOG.md has an entry for the `mediasoup` crate
		// version and grab its changes (used as the GitHub release body).
		try {
			versionChanges = await getVersionChanges(mediasoupVersion);
		} catch (error) {
			logError(error.message);

			exitWithError();
		}
	} else {
		logInfo(
			`checkRelease() | mediasoup ${mediasoupVersion} is already published, will not create a Git tag/release`
		);
	}

	// Validate packaging of the crates before the irreversible release steps
	// (git push, GitHub release, cargo publish).
	publishDryRun();

	return { tag, releaseMediasoup, versionChanges, cratesToPublish };
}

async function release() {
	logInfo('release()');

	// Make sure we are on the main branch.
	const branch = execSync('git rev-parse --abbrev-ref HEAD', {
		encoding: 'utf-8',
	}).trim();

	if (branch !== MAIN_BRANCH) {
		logError(
			`release() | must be on '${MAIN_BRANCH}' branch, but it is on '${branch}' branch`
		);

		exitWithError();
	}

	// Refuse to release with a dirty working tree. `cargo publish` would refuse
	// to publish modified local files anyway.
	checkGitClean();

	const releaseInfo = await checkRelease();

	// Nothing to release.
	if (!releaseInfo) {
		return;
	}

	const { tag, releaseMediasoup, versionChanges, cratesToPublish } =
		releaseInfo;

	// Push local commits first so everything we tag and publish is already on
	// GitHub.
	executeCmd(`git push origin ${MAIN_BRANCH}`);

	// Create the Git tag and the GitHub release only when the `mediasoup` crate
	// itself is being published. If the tag already exists (e.g. a previous run
	// was interrupted), skip this step but still publish below.
	if (releaseMediasoup && !gitTagExists(tag)) {
		let octokit;

		try {
			octokit = await getOctokit();
		} catch (error) {
			logError(error.message);

			exitWithError();
		}

		// Create and push the annotated tag.
		executeCmd(`git tag -a ${tag} -m ${tag}`);
		executeCmd(`git push origin ${tag}`);

		logInfo(`release() | creating release '${tag}' in GitHub`);

		await octokit.repos.createRelease({
			owner: GH_OWNER,
			repo: GH_REPO,
			name: tag,
			body: versionChanges,
			tag_name: tag,
			draft: false,
		});
	}

	// Publish the crates to crates.io. They are published in dependency order
	// (the order of `CRATES`). `--locked` makes cargo abort if Cargo.lock is out
	// of date instead of silently regenerating it.
	for (const crate of cratesToPublish) {
		executeInteractiveCmd('cargo publish --locked', { cwd: crate.dir });
	}
}

function lintRust() {
	logInfo('lintRust()');

	executeCmd('cargo fmt --all -- --check');
	executeCmd('cargo clippy --all-targets -- -D warnings');
}

function testRust() {
	logInfo('testRust()');

	executeCmd('cargo test --verbose');
	executeCmd('cargo test --release --verbose');
}

function docRust() {
	logInfo('docRust()');

	// Fail on broken/private intra-doc links, same as when building the docs for
	// docs.rs.
	process.env.DOCS_RS = '1';
	process.env.RUSTDOCFLAGS =
		'-D rustdoc::broken-intra-doc-links -D rustdoc::private_intra_doc_links';

	executeCmd('cargo doc --locked --all --no-deps --lib');
}

/**
 * Returns the subset of the given crates whose exact version is not yet
 * published on crates.io (so it has to be published).
 */
async function getUnpublishedCrates(crates) {
	const unpublishedCrates = [];

	for (const crate of crates) {
		if (!(await isCratePublished(crate.name, crate.version))) {
			unpublishedCrates.push(crate);
		}
	}

	return unpublishedCrates;
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

/**
 * Mimics getVersionChanges() in npm-scripts.mjs: looks up the heading matching
 * the given version in rust/CHANGELOG.md and returns the raw markdown of its
 * changes. Exits with error if there is no such entry.
 */
async function getVersionChanges(version) {
	logInfo(`getVersionChanges() [version:${version}]`);

	// NOTE: Load dep on demand since it's a devDependency.
	const marked = await import('marked');

	const changelog = fs.readFileSync(RUST_CHANGELOG, { encoding: 'utf-8' });
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
		`no entry found in ${RUST_CHANGELOG} for version '${version}'`
	);
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

async function getOctokit() {
	if (!process.env.GITHUB_TOKEN) {
		throw new Error('missing GITHUB_TOKEN environment variable');
	}

	// NOTE: Load dep on demand since it's a devDependency.
	const { Octokit } = await import('@octokit/rest');

	const octokit = new Octokit({
		auth: process.env.GITHUB_TOKEN,
	});

	return octokit;
}

function checkGitClean() {
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

function gitTagExists(tag) {
	try {
		execSync(`git rev-parse --verify --quiet refs/tags/${tag}`, {
			stdio: ['ignore', 'ignore', 'ignore'],
		});

		return true;
	} catch (error) {
		return false;
	}
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

function executeCmd(command) {
	logInfo(`executeCmd(): ${command}`);

	try {
		execSync(command, { stdio: ['ignore', process.stdout, process.stderr] });
	} catch (error) {
		logError(`executeCmd() failed, exiting: ${error}`);

		exitWithError();
	}
}

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
