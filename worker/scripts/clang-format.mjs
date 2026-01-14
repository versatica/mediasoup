import { execSync } from 'node:child_process';
import { glob } from 'glob';

const REQUIRED_CLANG_FORMAT_VERSION = 21;
const task = process.argv.slice(2).join(' ');

void run();

async function run() {
	const clangFormatBinary = getClangFormatBinary();

	checkClangFormatVersion(clangFormatBinary, REQUIRED_CLANG_FORMAT_VERSION);

	const workerFiles = await glob([
		'../src/**/*.cpp',
		'../include/**/*.hpp',
		'../test/src/**/*.cpp',
		'../test/include/**/**.hpp',
		'../fuzzer/src/**/*.cpp',
		'../fuzzer/include/**/*.hpp',
	]);

	switch (task) {
		case 'lint': {
			executeCmd(
				`"${clangFormatBinary}" --Werror --dry-run ${workerFiles.join(' ')}`
			);

			break;
		}

		case 'format': {
			executeCmd(`"${clangFormatBinary}" --Werror -i ${workerFiles.join(' ')}`);

			break;
		}

		default: {
			logError('unknown task');

			exitWithError();
		}
	}
}

function getClangFormatBinary() {
	let clangFormatBinary;

	if (process.env.MEDIASOUP_CLANG_FORMAT_BINARY) {
		clangFormatBinary = process.env.MEDIASOUP_CLANG_FORMAT_BINARY;

		logInfo(
			`getClangFormatBinary() | MEDIASOUP_CLANG_FORMAT_BINARY specified, using ${clangFormatBinary}`
		);

		return clangFormatBinary;
	}

	// Try `clang-format-XX` first, otherwise try `clang-format`.
	try {
		clangFormatBinary = `clang-format-${REQUIRED_CLANG_FORMAT_VERSION}`;

		execSync(`${clangFormatBinary} --version`, {
			stdio: ['ignore', 'ignore', 'ignore'],
		});

		logInfo(`getClangFormatBinary() | using ${clangFormatBinary}`);

		return clangFormatBinary;
	} catch {
		clangFormatBinary = 'clang-format';

		try {
			execSync(`${clangFormatBinary} --version`, {
				stdio: ['ignore', 'ignore', 'ignore'],
			});

			logInfo(`getClangFormatBinary() | using ${clangFormatBinary}`);

			return clangFormatBinary;
		} catch {
			logError('getClangFormatBinary() | clang-format binary not found');

			exitWithError();
		}
	}
}

function checkClangFormatVersion(clangFormatBinary, requiredVersion) {
	try {
		const version = getClangFormatVersion(clangFormatBinary);

		if (version === requiredVersion) {
			logInfo(
				`checkClangFormatVersion() | clang-format version is the required one (${requiredVersion})`
			);
		} else {
			logInfo(
				`checkClangFormatVersion() | clang-format version (${version}) is not the required one (${requiredVersion})`
			);

			exitWithError();
		}
	} catch (error) {
		logError(
			`checkClangFormatVersion() | failed to check clang-format version: ${error.message}`
		);

		exitWithError();
	}
}

function getClangFormatVersion(clangFormatBinary) {
	try {
		// Run the command and capture the output.
		const output = execSync(`${clangFormatBinary} --version`, {
			encoding: 'utf-8',
		});

		// Extract the mayor version number from the output.
		const match = output.match(/version (\d+)/);

		if (match && match[1]) {
			return parseInt(match[1], 10);
		} else {
			logError(
				`getClangFormatVersion() | unable to parse clang-format version: ${output}`
			);

			exitWithError();
		}
	} catch (error) {
		logError(
			`getClangFormatVersion() | error executing clang-format --version: ${error.message}`
		);

		exitWithError();
	}
}

function executeCmd(command) {
	try {
		execSync(command, { stdio: ['ignore', process.stdout, process.stderr] });
	} catch (error) {
		logError('executeCmd() failed');

		exitWithError();
	}
}

function logInfo(message) {
	// eslint-disable-next-line no-console
	console.log(`clang-format.mjs \x1b[36m[INFO] [${task}]\x1b[0m`, message);
}

// eslint-disable-next-line no-unused-vars
function logWarn(message) {
	// eslint-disable-next-line no-console
	console.warn(`clang-format.mjs \x1b[33m[WARN] [${task}]\x1b[0m`, message);
}

function logError(message) {
	// eslint-disable-next-line no-console
	console.error(`clang-format.mjs \x1b[31m[ERROR] [${task}]\x1b[0m`, message);
}

function exitWithError() {
	process.exit(1);
}
