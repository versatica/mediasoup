import * as process from 'node:process';
import * as fs from 'node:fs';
import { execSync } from 'node:child_process';

const task = process.argv[2];
const args = process.argv.slice(3).join(' ');

run();

async function run() {
	logInfo(args ? `[args:"${args}"]` : '');

	switch (task) {
		case 'binding:build': {
			logInfo(`binding:build: [args:"${args}"]`);
			buildBinding();

			break;
		}

		case 'typescript:build': {
			buildTypescript();

			break;
		}

		case 'typescript:watch': {
			deleteNodeLib();
			executeCmd(`tsc --project ./ --watch ${args}`);

			break;
		}

		case 'test': {
			buildTypescript();
			testNode();

			break;
		}

		default: {
			logError('unknown task');

			exitWithError();
		}
	}
}

function deleteNodeLib() {
	if (!fs.existsSync('node/lib')) {
		return;
	}

	logInfo('deleteNodeLib()');

	fs.rmSync('node/lib', { recursive: true, force: true });
}

function buildTypescript() {
	logInfo('buildTypescript()');

	deleteNodeLib();
	executeCmd('tsc --project ./');
}

function buildBinding() {
	logInfo('buildBinding()');

	executeCmd('npm ci --ignore-scripts');

	const buildType = process.env.MEDIASOUP_BUILDTYPE || 'Release';

	process.env.GYP_DEFINES = `mediasoup_build_type=${buildType}`;

	if (process.env.MEDIASOUP_WORKER_LIB) {
		process.env.GYP_DEFINES += ` mediasoup_worker_lib=${process.env.MEDIASOUP_WORKER_LIB}`;
	}

	executeCmd(`node-gyp rebuild --${buildType.toLowerCase()} --verbose`);
}

function testNode() {
	logInfo('testNode()');

	executeCmd('node --test-only lib/test.js');
}

function executeCmd(command, exitOnError = true) {
	logInfo(`executeCmd(): ${command}`);

	try {
		execSync(command, {
			stdio: ['ignore', process.stdout, process.stderr],
		});
	} catch (error) {
		if (exitOnError) {
			logError(`executeCmd() failed, exiting: ${error}`);

			exitWithError();
		} else {
			logInfo(`executeCmd() failed, ignoring: ${error}`);
		}
	}
}

function logInfo(message) {
	// eslint-disable-next-line no-console
	console.log(`npm-scripts.mjs \x1b[36m[INFO] [${task}]\x1b[0m`, message);
}

// eslint-disable-next-line no-unused-vars
function logWarn(message) {
	// eslint-disable-next-line no-console
	console.warn(`npm-scripts.mjs \x1b[33m[WARN] [${task}]\x1b\0m`, message);
}

function logError(message) {
	// eslint-disable-next-line no-console
	console.error(`npm-scripts.mjs \x1b[31m[ERROR] [${task}]\x1b[0m`, message);
}

function exitWithError() {
	process.exit(1);
}
