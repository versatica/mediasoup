import * as process from 'node:process';
import * as os from 'node:os';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { execSync } from 'node:child_process';
import fetch from 'node-fetch';
import * as tar from 'tar';
import * as ini from 'ini';
import pkg from './package.json' with { type: 'json' };

const IS_WINDOWS = os.platform() === 'win32';
const MAYOR_VERSION = pkg.version.split('.')[0];
const PYTHON = getPython();
const PIP_INVOKE_DIR = path.resolve('worker/pip_invoke');
const WORKER_RELEASE_DIR = 'worker/out/Release';
const WORKER_RELEASE_BIN = IS_WINDOWS
	? 'mediasoup-worker.exe'
	: 'mediasoup-worker';
const WORKER_RELEASE_BIN_PATH = `${WORKER_RELEASE_DIR}/${WORKER_RELEASE_BIN}`;
const WORKER_PREBUILD_DIR = 'worker/prebuild';
const GH_OWNER = 'versatica';
const GH_REPO = 'mediasoup';

// Paths for ESLint to check. Converted to string for convenience.
const ESLINT_PATHS = [
	'eslint.config.mjs',
	'jest.config.mjs',
	'knip.config.mjs',
	'node/src',
	'npm-scripts.mjs',
	'worker/scripts',
].join(' ');

// Paths for ESLint to ignore. Converted to string argument for convenience.
const ESLINT_IGNORE_PATTERN_ARGS = ['node/src/fbs']
	.map(entry => `--ignore-pattern ${entry}`)
	.join(' ');

// Paths for Prettier to check/write. Converted to string for convenience.
// NOTE: Prettier ignores paths in .gitignore so we don't need to care about
// node/src/fbs.
const PRETTIER_PATHS = [
	'CHANGELOG.md',
	'CONTRIBUTING.md',
	'README.md',
	'doc',
	'eslint.config.mjs',
	'jest.config.mjs',
	'knip.config.mjs',
	'node/src',
	'npm-scripts.mjs',
	'package.json',
	'tsconfig.json',
	'worker/scripts',
].join(' ');

const task = process.argv[2];
const taskArgs = process.argv.slice(3).join(' ');

// PYTHONPATH env must be updated now so all invoke calls below will find the
// pip invoke module.
if (process.env.PYTHONPATH) {
	if (IS_WINDOWS) {
		process.env.PYTHONPATH = `${PIP_INVOKE_DIR};${process.env.PYTHONPATH}`;
	} else {
		process.env.PYTHONPATH = `${PIP_INVOKE_DIR}:${process.env.PYTHONPATH}`;
	}
} else {
	process.env.PYTHONPATH = PIP_INVOKE_DIR;
}

void run();

async function run() {
	logInfo(taskArgs ? `[args:"${taskArgs}"]` : '');

	switch (task) {
		// As per NPM documentation (https://docs.npmjs.com/cli/v9/using-npm/scripts)
		// `prepare` script:
		//
		// - Runs BEFORE the package is packed, i.e. during `npm publish` and
		//   `npm pack`.
		// - Runs on local `npm install` without any arguments.
		// - NOTE: If a package being installed through git contains a `prepare`
		//   script, its dependencies and devDependencies will be installed, and
		//   the `prepare` script will be run, before the package is packaged and
		//   installed.
		//
		// So here we generate flatbuffers definitions for TypeScript and compile
		// TypeScript to JavaScript.
		case 'prepare': {
			flatcNode();
			buildTypescript({ force: false });

			break;
		}

		case 'postinstall': {
			// If the user/app provides us with a custom mediasoup-worker binary then
			// don't do anything.
			if (process.env.MEDIASOUP_WORKER_BIN) {
				logInfo('MEDIASOUP_WORKER_BIN environment variable given, skipping');

				break;
			}
			// If MEDIASOUP_LOCAL_DEV is given, or if MEDIASOUP_SKIP_WORKER_PREBUILT_DOWNLOAD
			// env is given, or if mediasoup package is being installed via git+ssh
			// (instead of via npm), and if MEDIASOUP_FORCE_PREBUILT_WORKER_DOWNLOAD
			// env is not set, then skip mediasoup-worker prebuilt download.
			else if (
				(process.env.MEDIASOUP_LOCAL_DEV ||
					process.env.MEDIASOUP_SKIP_WORKER_PREBUILT_DOWNLOAD ||
					process.env.npm_package_resolved?.startsWith('git+ssh://')) &&
				!process.env.MEDIASOUP_FORCE_WORKER_PREBUILT_DOWNLOAD
			) {
				logInfo(
					'skipping mediasoup-worker prebuilt download, building it locally'
				);

				buildWorker();

				if (!process.env.MEDIASOUP_LOCAL_DEV) {
					cleanWorkerArtifacts();
				}
			}
			// Attempt to download a prebuilt binary. Fallback to building locally.
			else if (!(await downloadPrebuiltWorker())) {
				logInfo(
					`couldn't fetch any mediasoup-worker prebuilt binary, building it locally`
				);

				buildWorker();

				if (!process.env.MEDIASOUP_LOCAL_DEV) {
					cleanWorkerArtifacts();
				}
			}

			break;
		}

		case 'typescript:build': {
			buildTypescript({ force: true });

			break;
		}

		case 'typescript:watch': {
			watchTypescript();

			break;
		}

		case 'worker:build': {
			buildWorker();

			break;
		}

		case 'worker:prebuild-name': {
			getWorkerPrebuildTarName();

			break;
		}

		case 'worker:prebuild': {
			await prebuildWorker();

			break;
		}

		case 'lint:node': {
			lintNode();

			break;
		}

		case 'lint:worker': {
			lintWorker();

			break;
		}

		case 'format:node': {
			formatNode();

			break;
		}

		case 'format:worker': {
			formatWorker();

			break;
		}

		case 'flatc:node': {
			flatcNode();

			break;
		}

		case 'flatc:worker': {
			flatcWorker();

			break;
		}

		case 'test:node': {
			testNode();

			break;
		}

		case 'test:worker': {
			testWorker();

			break;
		}

		case 'coverage:node': {
			coverageNode();

			break;
		}

		case 'release:check': {
			checkRelease();

			break;
		}

		case 'release': {
			release();

			break;
		}

		default: {
			logError('unknown task');

			exitWithError();
		}
	}
}

function getPython() {
	let python = process.env.PYTHON;

	if (!python) {
		try {
			execSync('python3 --version', { stdio: ['ignore', 'ignore', 'ignore'] });
			python = 'python3';
		} catch (error) {
			python = 'python';
		}
	}

	return python;
}

function getWorkerPrebuildTarName() {
	let workerPrebuildTarName = `mediasoup-worker-${pkg.version}-${os.platform()}-${os.arch()}`;

	// In Linux we want to know about kernel version since kernel >= 6 supports
	// io-uring.
	if (os.platform() === 'linux') {
		const kernelMajorVersion = Number(os.release().split('.')[0]);

		workerPrebuildTarName += `-kernel${kernelMajorVersion}`;
	}

	workerPrebuildTarName = `${workerPrebuildTarName}.tgz`;

	logInfo(
		`getWorkerPrebuildTarName() [workerPrebuildTarName:${workerPrebuildTarName}]`
	);

	return workerPrebuildTarName;
}

function installInvoke() {
	if (fs.existsSync(PIP_INVOKE_DIR)) {
		return;
	}

	logInfo('installInvoke()');

	// Install pip invoke into custom location, so we don't depend on system-wide
	// installation.
	executeCmd(
		`"${PYTHON}" -m pip install --upgrade --no-user --target "${PIP_INVOKE_DIR}" invoke`
	);
}

function deleteNodeLib() {
	if (!fs.existsSync('node/lib')) {
		return;
	}

	logInfo('deleteNodeLib()');

	fs.rmSync('node/lib', { recursive: true, force: true });
}

function buildTypescript({ force }) {
	if (!force && fs.existsSync('node/lib')) {
		return;
	}

	logInfo('buildTypescript()');

	deleteNodeLib();

	executeCmd(`tsc ${taskArgs}`);
}

function watchTypescript() {
	logInfo('watchTypescript()');

	deleteNodeLib();

	executeCmd(`tsc --watch ${taskArgs}`);
}

function buildWorker() {
	logInfo('buildWorker()');

	installInvoke();

	executeCmd(`"${PYTHON}" -m invoke -r worker mediasoup-worker`);
}

function cleanWorkerArtifacts() {
	logInfo('cleanWorkerArtifacts()');

	installInvoke();

	// Clean build artifacts except `mediasoup-worker`.
	executeCmd(`"${PYTHON}" -m invoke -r worker clean-build`);

	// Clean downloaded dependencies.
	executeCmd(`"${PYTHON}" -m invoke -r worker clean-subprojects`);

	// Clean PIP/Meson/Ninja.
	executeCmd(`"${PYTHON}" -m invoke -r worker clean-pip`);
}

function lintNode() {
	logInfo('lintNode()');

	// Ensure there are no rules that are unnecessary or conflict with Prettier
	// rules.
	executeCmd('eslint-config-prettier eslint.config.mjs');

	executeCmd(
		`eslint -c eslint.config.mjs --max-warnings 0 ${ESLINT_IGNORE_PATTERN_ARGS} ${ESLINT_PATHS}`
	);

	executeCmd(`prettier --check ${PRETTIER_PATHS}`);

	executeCmd('knip --config knip.config.mjs --treat-config-hints-as-errors');
}

function lintWorker() {
	logInfo('lintWorker()');

	installInvoke();

	executeCmd(`"${PYTHON}" -m invoke -r worker lint`);
}

function formatNode() {
	logInfo('formatNode()');

	executeCmd(`prettier --write ${PRETTIER_PATHS}`);
}

function formatWorker() {
	logInfo('formatWorker()');

	installInvoke();

	executeCmd(`"${PYTHON}" -m invoke -r worker format`);
}

function flatcNode() {
	logInfo('flatcNode()');

	installInvoke();

	// Build flatc if needed.
	executeCmd(`"${PYTHON}" -m invoke -r worker flatc`);

	const buildType = process.env.MEDIASOUP_BUILDTYPE || 'Release';
	const extension = IS_WINDOWS ? '.exe' : '';
	const flatbuffersWrapFilePath = path.join(
		'worker',
		'subprojects',
		'flatbuffers.wrap'
	);
	const flatbuffersWrap = ini.parse(
		fs.readFileSync(flatbuffersWrapFilePath, {
			encoding: 'utf-8',
		})
	);
	const flatbuffersDir = flatbuffersWrap['wrap-file']['directory'];

	const flatc = path.resolve(
		path.join(
			'worker',
			'out',
			buildType,
			'build',
			'subprojects',
			flatbuffersDir,
			`flatc${extension}`
		)
	);

	const out = path.resolve(path.join('node', 'src'));

	for (const dirent of fs.readdirSync(path.join('worker', 'fbs'), {
		withFileTypes: true,
	})) {
		if (!dirent.isFile() || path.parse(dirent.name).ext !== '.fbs') {
			continue;
		}

		const filePath = path.resolve(path.join('worker', 'fbs', dirent.name));

		executeCmd(
			`"${flatc}" --ts --ts-no-import-ext --gen-object-api -o "${out}" "${filePath}"`
		);
	}
}

function flatcWorker() {
	logInfo('flatcWorker()');

	installInvoke();

	executeCmd(`"${PYTHON}" -m invoke -r worker flatc`);
}

function testNode() {
	logInfo('testNode()');

	executeCmd(`jest --silent false --detectOpenHandles ${taskArgs}`);
}

function testWorker() {
	logInfo('testWorker()');

	installInvoke();

	executeCmd(`"${PYTHON}" -m invoke -r worker test`);
}

function coverageNode() {
	logInfo('coverageNode()');

	executeCmd(`jest --coverage ${taskArgs}`);
	executeCmd('open-cli coverage/lcov-report/index.html');
}

function installNodeDeps() {
	logInfo('installNodeDeps()');

	// Install/update Node deps.
	executeCmd('npm ci --ignore-scripts');

	// Update package-lock.json.
	executeCmd('npm install --package-lock-only --ignore-scripts');

	// Check vulnerabilities in deps (exclude dev deps).
	executeCmd('npm audit --omit=dev');
}

function checkRelease() {
	logInfo('checkRelease()');

	installNodeDeps();
	flatcNode();
	buildTypescript({ force: true });
	buildWorker();
	lintNode();
	lintWorker();
	testNode();
	testWorker();
}

async function release() {
	logInfo('release()');

	let octokit;
	let versionChanges;

	try {
		octokit = await getOctokit();
		versionChanges = await getVersionChanges();
	} catch (error) {
		logError(error.message);

		exitWithError();
	}

	checkRelease();
	executeCmd(`git commit -am '${pkg.version}'`);
	executeCmd(`git tag -a ${pkg.version} -m '${pkg.version}'`);
	executeCmd(`git push origin v${MAYOR_VERSION}`);
	executeCmd(`git push origin '${pkg.version}'`);

	logInfo('creating release in GitHub');

	await octokit.repos.createRelease({
		owner: GH_OWNER,
		repo: GH_REPO,
		name: pkg.version,
		body: versionChanges,
		tag_name: pkg.version,
		draft: false,
	});

	executeInteractiveCmd('npm publish');
}

function ensureDir(dir) {
	logInfo(`ensureDir() [dir:${dir}]`);

	if (!fs.existsSync(dir)) {
		fs.mkdirSync(dir, { recursive: true });
	}
}

async function prebuildWorker() {
	logInfo('prebuildWorker()');

	ensureDir(WORKER_PREBUILD_DIR);

	const workerPrebuildTar = getWorkerPrebuildTarName();
	const workerPrebuildTarPath = `${WORKER_PREBUILD_DIR}/${workerPrebuildTar}`;

	try {
		await new Promise((resolve, reject) => {
			// Generate a gzip file which just contains mediasoup-worker binary
			// without any folder.
			tar
				.create(
					{
						cwd: WORKER_RELEASE_DIR,
						gzip: true,
						strict: true,
					},
					[WORKER_RELEASE_BIN]
				)
				// This is needed for the case in which tar.create() fails before
				// invoking pipe() on its result.
				.on('error', reject)
				.pipe(fs.createWriteStream(workerPrebuildTarPath))
				.on('finish', resolve)
				.on('error', reject);
		});
	} catch (error) {
		logError(
			'prebuildWorker() | failed to create mediasoup-worker prebuilt tar file:',
			error
		);

		exitWithError();
	}
}

// Returns a Promise resolving to true if a mediasoup-worker prebuilt binary
// was downloaded and uncompressed, false otherwise.
async function downloadPrebuiltWorker() {
	const releaseBase =
		process.env.MEDIASOUP_WORKER_PREBUILT_DOWNLOAD_BASE_URL ||
		`${pkg.repository.url
			.replace(/^git\+/, '')
			.replace(/\.git$/, '')}/releases/download`;

	const workerPrebuildTar = getWorkerPrebuildTarName();
	const workerPrebuildTarUrl = `${releaseBase}/${pkg.version}/${workerPrebuildTar}`;

	logInfo(
		`downloadPrebuiltWorker() [workerPrebuildTarUrl:${workerPrebuildTarUrl}]`
	);

	ensureDir(WORKER_PREBUILD_DIR);

	let res;

	try {
		res = await fetch(workerPrebuildTarUrl);

		if (res.status === 404) {
			logInfo(
				'downloadPrebuiltWorker() | no available mediasoup-worker prebuilt binary for current architecture'
			);

			return false;
		} else if (!res.ok) {
			logError(
				`downloadPrebuiltWorker() | failed to download mediasoup-worker prebuilt binary: ${res.status} ${res.statusText}`
			);

			return false;
		}
	} catch (error) {
		logError(
			`downloadPrebuiltWorker() | failed to download mediasoup-worker prebuilt binary: ${error}`
		);

		return false;
	}

	ensureDir(WORKER_RELEASE_DIR);

	return new Promise(resolve => {
		// Extract mediasoup-worker in the official mediasoup-worker path.
		res.body
			.pipe(
				tar.extract({
					cwd: WORKER_RELEASE_DIR,
					newer: false,
					strict: true,
				})
			)
			.on('finish', () => {
				logInfo(
					'downloadPrebuiltWorker() | got mediasoup-worker prebuilt binary'
				);

				try {
					// Give execution permission to the binary.
					fs.chmodSync(WORKER_RELEASE_BIN_PATH, 0o775);
				} catch (error) {
					logWarn(
						`downloadPrebuiltWorker() | failed to give execution permissions to the mediasoup-worker prebuilt binary: ${error}`
					);
				}

				// Let's confirm that the fetched mediasoup-worker prebuit binary does
				// run in current host. This is to prevent weird issues related to
				// different versions of libc in the system and so on.
				// So run mediasoup-worker without the required MEDIASOUP_VERSION env
				// and expect exit code 41 (see main.cpp).

				logInfo(
					'downloadPrebuiltWorker() | checking fetched mediasoup-worker prebuilt binary in current host'
				);

				try {
					const resolvedBinPath = path.resolve(WORKER_RELEASE_BIN_PATH);

					// This will always fail on purpose, but if status code is 41 then
					// it's good.
					execSync(`"${resolvedBinPath}"`, {
						stdio: ['ignore', 'ignore', 'ignore'],
						// Ensure no env is passed to avoid accidents.
						env: {},
					});
				} catch (error) {
					if (error.status === 41) {
						logInfo(
							'downloadPrebuiltWorker() | fetched mediasoup-worker prebuilt binary is valid for current host'
						);

						resolve(true);
					} else {
						logError(
							`downloadPrebuiltWorker() | fetched mediasoup-worker prebuilt binary fails to run in this host [status:${error.status}]`
						);

						try {
							fs.unlinkSync(WORKER_RELEASE_BIN_PATH);
						} catch (error2) {}

						resolve(false);
					}
				}
			})
			.on('error', error => {
				logError(
					`downloadPrebuiltWorker() | failed to extract downloaded mediasoup-worker prebuilt binary:`,
					error
				);

				resolve(false);
			});
	});
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

async function getVersionChanges() {
	logInfo('getVersionChanges()');

	// NOTE: Load dep on demand since it's a devDependency.
	const marked = await import('marked');

	const changelog = fs.readFileSync('./CHANGELOG.md', { encoding: 'utf-8' });
	const entries = marked.lexer(changelog);

	for (let idx = 0; idx < entries.length; ++idx) {
		const entry = entries[idx];

		if (entry.type === 'heading' && entry.text === pkg.version) {
			const changes = entries[idx + 1].raw;

			return changes;
		}
	}

	// This should not happen (unless author forgot to update CHANGELOG).
	throw new Error(
		`no entry found in CHANGELOG.md for version '${pkg.version}'`
	);
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

function executeInteractiveCmd(command) {
	logInfo(`executeInteractiveCmd(): ${command}`);

	try {
		execSync(command, { stdio: 'inherit', env: process.env });
	} catch (error) {
		logError(`executeInteractiveCmd() failed, exiting: ${error}`);

		exitWithError();
	}
}

function logInfo(...args) {
	// eslint-disable-next-line no-console
	console.log(`npm-scripts.mjs \x1b[36m[INFO] [${task}]\x1b[0m`, ...args);
}

function logWarn(...args) {
	// eslint-disable-next-line no-console
	console.warn(`npm-scripts.mjs \x1b[33m[WARN] [${task}]\x1b\0m`, ...args);
}

function logError(...args) {
	// eslint-disable-next-line no-console
	console.error(`npm-scripts.mjs \x1b[31m[ERROR] [${task}]\x1b[0m`, ...args);
}

function exitWithError() {
	process.exit(1);
}
