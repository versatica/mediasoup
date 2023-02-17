/* eslint-disable no-console */

const process = require('process');
const os = require('os');
const fs = require('fs');
const { execSync, spawnSync } = require('child_process');
const { version } = require('./package.json');

const isFreeBSD = os.platform() === 'freebsd';
const isWindows = os.platform() === 'win32';
const task = process.argv.slice(2).join(' ');

// mediasoup mayor version.
const MAYOR_VERSION = version.split('.')[0];

// make command to use.
const MAKE = process.env.MAKE || (isFreeBSD ? 'gmake' : 'make');

console.log(`npm-scripts.js [INFO] running task "${task}"`);

switch (task)
{
	// As per NPM documentation (https://docs.npmjs.com/cli/v9/using-npm/scripts)
	// `prepare` script:
	//
	// - Runs BEFORE the package is packed, i.e. during `npm publish` and `npm pack`.
	// - Runs on local `npm install` without any arguments.
	// - NOTE: If a package being installed through git contains a `prepare` script,
	//   its dependencies and devDependencies will be installed, and the `prepare`
	//   script will be run, before the package is packaged and installed.
	//
	// So here we compile TypeScript and flatbuffers to JavaScript.
	case 'prepare':
	{
		buildTypescript(/* force */ false);

		// TODO: Compile flatbuffers.

		break;
	}

	case 'postinstall':
	{
		if (!process.env.MEDIASOUP_WORKER_BIN)
		{
			buildWorker();

			if (!process.env.MEDIASOUP_LOCAL_DEV)
			{
				cleanWorker();
			}
		}

		break;
	}

	case 'typescript:build':
	{
		installNodeDeps();
		buildTypescript(/* force */ true);
		replaceVersion();

		break;
	}

	case 'typescript:watch':
	{
		deleteNodeLib();

		const { TscWatchClient } = require('tsc-watch/client');
		const watch = new TscWatchClient();

		watch.on('success', replaceVersion);
		watch.start('--project', 'node', '--pretty');

		break;
	}

	case 'worker:build':
	{
		buildWorker();

		break;
	}

	case 'lint:node':
	{
		lintNode();

		break;
	}

	case 'lint:worker':
	{
		lintWorker();

		break;
	}

	case 'format:worker':
	{
		executeCmd(`${MAKE} format -C worker`);

		break;
	}

	case 'test:node':
	{
		buildTypescript(/* force */ false);
		replaceVersion();
		testNode();

		break;
	}

	case 'test:worker':
	{
		testWorker();

		break;
	}

	case 'coverage:node':
	{
		buildTypescript(/* force */ false);
		replaceVersion();
		executeCmd('jest --coverage');
		executeCmd('open-cli coverage/lcov-report/index.html');

		break;
	}

	case 'install-deps:node':
	{
		installNodeDeps();

		break;
	}

	case 'install-clang-tools':
	{
		executeCmd('npm ci --prefix worker/scripts');

		break;
	}

	case 'release:check':
	{
		checkRelease();

		break;
	}

	case 'release':
	{
		checkRelease();
		executeCmd(`git commit -am '${version}'`);
		executeCmd(`git tag -a ${version} -m '${version}'`);
		executeCmd(`git push origin v${MAYOR_VERSION}`);
		executeCmd(`git push origin '${version}'`);
		executeCmd('npm publish');

		break;
	}

	default:
	{
		throw new TypeError(`unknown task "${task}"`);
	}
}

function replaceVersion()
{
	console.log('npm-scripts.js [INFO] replaceVersion()');

	const files =
	[
		'node/lib/index.js',
		'node/lib/index.d.ts',
		'node/lib/Worker.js'
	];

	for (const file of files)
	{
		const text = fs.readFileSync(file, { encoding: 'utf8' });
		const result = text.replace(/__MEDIASOUP_VERSION__/g, version);

		fs.writeFileSync(file, result, { encoding: 'utf8' });
	}
}

function deleteNodeLib()
{
	if (!fs.existsSync('node/lib'))
	{
		return;
	}

	console.log('npm-scripts.js [INFO] deleteNodeLib()');

	if (!isWindows)
	{
		executeCmd('rm -rf node/lib');
	}
	else
	{
		// NOTE: This command fails in Windows if the dir doesn't exist.
		executeCmd('rmdir /s /q "node/lib"', /* exitOnError */ false);
	}
}

function buildTypescript(force = false)
{
	if (!force && fs.existsSync('node/lib'))
	{
		return;
	}

	console.log('npm-scripts.js [INFO] buildTypescript()');

	deleteNodeLib();

	executeCmd('tsc --project node');
}

function buildWorker()
{
	console.log('npm-scripts.js [INFO] buildWorker()');

	if (isWindows)
	{
		if (!fs.existsSync('worker/out/msys/bin/make.exe'))
		{
			installMsysMake();
		}

		const msysPath = `${process.cwd()}\\worker\\out\\msys\\bin`;

		if (!process.env['PATH'].includes(msysPath))
		{
			process.env['PATH'] = `${msysPath};${process.env['PATH']}`;
		}
	}

	executeCmd(`${MAKE} -C worker`);
}

function cleanWorker()
{
	console.log('npm-scripts.js [INFO] cleanWorker()');

	// Clean build artifacts except `mediasoup-worker`.
	executeCmd(`${MAKE} clean-build -C worker`);
	// Clean downloaded dependencies.
	executeCmd(`${MAKE} clean-subprojects -C worker`);
	// Clean PIP/Meson/Ninja.
	executeCmd(`${MAKE} clean-pip -C worker`);

	if (isWindows)
	{
		executeCmd('rd /s /q worker\\out\\msys');
	}
}

function lintNode()
{
	console.log('npm-scripts.js [INFO] lintNode()');

	executeCmd('eslint -c node/.eslintrc.js --max-warnings 0 node/src node/.eslintrc.js npm-scripts.js worker/scripts/gulpfile.js');
}

function lintWorker()
{
	console.log('npm-scripts.js [INFO] lintWorker()');

	executeCmd(`${MAKE} lint -C worker`);
}

function testNode()
{
	console.log('npm-scripts.js [INFO] testNode()');

	if (!process.env.TEST_FILE)
	{
		executeCmd('jest');
	}
	else
	{
		executeCmd(`jest --testPathPattern ${process.env.TEST_FILE}`);
	}
}

function testWorker()
{
	console.log('npm-scripts.js [INFO] testWorker()');

	executeCmd(`${MAKE} test -C worker`);
}

function installNodeDeps()
{
	console.log('npm-scripts.js [INFO] installNodeDeps()');

	// Install/update Node deps.
	executeCmd('npm ci --ignore-scripts');
	// Update package-lock.json.
	executeCmd('npm install --package-lock-only --ignore-scripts');
}

function checkRelease()
{
	console.log('npm-scripts.js [INFO] checkRelease()');

	installNodeDeps();
	buildTypescript(/* force */ true);
	replaceVersion();
	buildWorker();
	lintNode();
	lintWorker();
	testNode();
	testWorker();
}

function executeCmd(command, exitOnError = true)
{
	console.log(`npm-scripts.js [INFO] executeCmd(): ${command}`);

	try
	{
		execSync(command, { stdio: [ 'ignore', process.stdout, process.stderr ] });
	}
	catch (error)
	{
		if (exitOnError)
		{
			console.error(`npm-scripts.js [ERROR] executeCmd() failed, exiting: ${error}`);

			process.exit(1);
		}
		else
		{
			console.log(`npm-scripts.js [INFO] executeCmd() failed, ignoring: ${error}`);
		}
	}
}

function installMsysMake()
{
	console.log('npm-scripts.js [INFO] installMsysMake()');

	let res = spawnSync('where', [ 'python3.exe' ]);

	if (res.status !== 0)
	{
		res = spawnSync('where', [ 'python.exe' ]);

		if (res.status !== 0)
		{
			// eslint-disable-next-line no-console
			console.error('`npm-scripts.js [ERROR] installMsysMake() cannot find Python executable');

			process.exit(1);
		}
	}

	executeCmd(`${String(res.stdout).trim()} worker\\scripts\\getmake.py`);
}
