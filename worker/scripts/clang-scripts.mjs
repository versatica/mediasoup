import * as path from 'node:path';
import * as os from 'node:os';
import { execSync } from 'node:child_process';
import { glob } from 'glob';

const PYTHON = getPython();
const BUILD_DIR = getBuildDir();
const NUM_CORES = getNumCores();
const CLANG_FORMAT_VERSION = 21;
const CLANG_TIDY_VERSION = 21;

const CLANG_FORMAT_PATHS = await glob([
	'../src/**/*.cpp',
	'../include/**/*.hpp',
	'../test/src/**/*.cpp',
	'../test/include/**/**.hpp',
	'../fuzzer/src/**/*.cpp',
	'../fuzzer/include/**/*.hpp',
]);

// TODO: Enable test/ and fuzzer/ source files.
const CLANG_TIDY_PATHS = await glob([
	'../src/**/*.cpp',
	// '../test/src/**/*.cpp',
	// '../fuzzer/src/**/*.cpp',
]);

const task = process.argv.slice(2).join(' ');

void run();

async function run() {
	switch (task) {
		case 'lint': {
			lint();

			break;
		}

		case 'format': {
			format();

			break;
		}

		case 'tidy': {
			tidy();

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

	const clangFormat = getClangToolBinary({
		clangToolName: 'clang-format',
		version: CLANG_FORMAT_VERSION,
		checkRequireVersion: true,
	});

	executeCmd(
		`"${clangFormat}" --Werror --dry-run ${CLANG_FORMAT_PATHS.join(' ')}`
	);
}

function format() {
	logInfo('format()');

	const clangFormat = getClangToolBinary({
		clangToolName: 'clang-format',
		version: CLANG_FORMAT_VERSION,
		checkRequireVersion: true,
	});

	const files = CLANG_FORMAT_PATHS.join(' ');

	executeCmd(`"${clangFormat}" --Werror -i ${files}`);
}

function tidy() {
	logInfo('tidy()');

	const clangTidy = getClangToolBinary({
		clangToolName: 'clang-tidy',
		version: CLANG_TIDY_VERSION,
		checkRequireVersion: true,
	});

	const runClangTidy = getClangToolBinary({
		clangToolName: 'run-clang-tidy',
		version: CLANG_TIDY_VERSION,
		// Don't check version because this command does not provide --version
		// option.
		checkRequireVersion: false,
	});

	const clangApplyReplacements = getClangToolBinary({
		clangToolName: 'clang-apply-replacements',
		version: CLANG_TIDY_VERSION,
		checkRequireVersion: true,
	});

	const tidyChecks = process.env.MEDIASOUP_TIDY_CHECKS
		? `-*,${process.env.MEDIASOUP_TIDY_CHECKS}`
		: '';

	const files = process.env.MEDIASOUP_TIDY_FILES ?? CLANG_TIDY_PATHS.join(' ');

	executeCmd(
		`"${PYTHON}" "${runClangTidy}" -clang-tidy-binary="${clangTidy}" -clang-apply-replacements-binary="${clangApplyReplacements}" -p="${BUILD_DIR}" -j=${NUM_CORES} -quiet -fix -format -checks=${tidyChecks} ${files}`
	);
}

function getClangToolBinary({ clangToolName, version, checkRequireVersion }) {
	logInfo(
		`getClangToolBinary() [clangToolName:${clangToolName}, version:${version}]`
	);

	let clangToolBinary;

	// Try `clangTool-version` first, otherwise try `clangTool`.
	try {
		clangToolBinary = `${clangToolName}-${version}`;

		logInfo(`getClangToolBinary() | trying ${clangToolBinary}...`);

		execSync(`${clangToolBinary} --help`, {
			stdio: ['ignore', 'ignore', 'ignore'],
		});
	} catch (error) {
		clangToolBinary = clangToolName;

		logInfo(`getClangToolBinary() | trying ${clangToolBinary}...`);

		try {
			execSync(`${clangToolBinary} --help`, {
				stdio: ['ignore', 'ignore', 'ignore'],
			});
		} catch {
			logError(`getClangToolBinary() | ${clangToolName} binary not found`);

			exitWithError();
		}
	}

	logInfo(`getClangToolBinary() | using ${clangToolBinary}`);

	if (checkRequireVersion) {
		try {
			checkClangToolVersion(clangToolBinary, version);
		} catch (error) {
			logError(`getClangToolBinary() | failed: ${error.message}`);

			exitWithError();
		}
	}

	const clangToolBinaryAbsolutePath = execSync(`which ${clangToolBinary}`, {
		encoding: 'utf8',
	}).trim();

	return clangToolBinaryAbsolutePath;
}

function checkClangToolVersion(clangToolBinary, requiredVersion) {
	try {
		let version;

		// Run the command and capture the output.
		const output = execSync(`${clangToolBinary} --version`, {
			encoding: 'utf-8',
		});

		// Extract the mayor version number from the output.
		const match = output.match(/version (\d+)/);

		if (match && match[1]) {
			version = parseInt(match[1], 10);
		} else {
			throw new Error(
				`checkClangToolVersion() | unable to parse output of '${clangToolBinary} --version': ${output}`
			);
		}

		if (version === requiredVersion) {
			logInfo(
				`checkClangToolVersion() | ${clangToolBinary} version is the required one (${requiredVersion})`
			);
		} else {
			throw new Error(
				`checkClangToolVersion() | ${clangToolBinary} version (${version}) is not the required one (${requiredVersion})`
			);
		}
	} catch (error) {
		throw new Error(
			`checkClangToolVersion() | failed to check ${clangToolBinary} version: ${error.message}`
		);
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

function getBuildDir() {
	const mediasoupBuildtype = process.env.MEDIASOUP_BUILDTYPE ?? 'Release';
	const workerDir = path.resolve(path.join('../'));
	const workerOutDir = process.env.MEDIASOUP_OUT_DIR ?? `${workerDir}/out`;
	const workerInstallDir =
		process.env.MEDIASOUP_INSTALL_DIR ??
		`${workerOutDir}/${mediasoupBuildtype}`;
	const buildDir = process.envBUILD_DIR ?? `${workerInstallDir}/build`;

	return buildDir;
}

function getNumCores() {
	return Object.keys(os.cpus()).length;
}

function executeCmd(command) {
	logInfo(`executeCmd(): ${command}`);

	try {
		execSync(command, { stdio: ['ignore', process.stdout, process.stderr] });
	} catch (error) {
		logError('executeCmd() failed');

		exitWithError();
	}
}

function logInfo(message) {
	// eslint-disable-next-line no-console
	console.log(`clang-scripts.mjs \x1b[36m[INFO] [${task}]\x1b[0m`, message);
}

// eslint-disable-next-line no-unused-vars
function logWarn(message) {
	// eslint-disable-next-line no-console
	console.warn(`clang-scripts.mjs \x1b[33m[WARN] [${task}]\x1b[0m`, message);
}

function logError(message) {
	// eslint-disable-next-line no-console
	console.error(`clang-scripts.mjs \x1b[31m[ERROR] [${task}]\x1b[0m`, message);
}

function exitWithError() {
	process.exit(1);
}
