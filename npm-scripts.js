/* eslint-disable no-console */

const process = require('process');
const os = require('os');
const fs = require('fs');
const { execSync } = require('child_process');
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
	// As per NPM documentation (https://docs.npmjs.com/cli/v9/using-npm/scripts):
	//
	// If a package being installed through git contains a prepare script, its
	// dependencies and devDependencies will be installed, and the prepare script
	// will be run, before the package is packaged and installed.
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
		buildTypescript(/* force */ true);
		replaceVersion();

		break;
	}

	case 'typescript:watch':
	{
		deleteNodeLib();

		const TscWatchClient = require('tsc-watch/client');
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
		executeCmd('eslint -c node/.eslintrc.js --max-warnings 0 node/src/ node/.eslintrc.js npm-scripts.js node/tests/ worker/scripts/gulpfile.js');

		break;
	}

	case 'lint:worker':
	{
		executeCmd(`${MAKE} lint -C worker`);

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

		if (!process.env.TEST_FILE)
		{
			executeCmd('jest');
		}
		else
		{
			executeCmd(`jest --testPathPattern ${process.env.TEST_FILE}`);
		}

		break;
	}

	case 'test:worker':
	{
		executeCmd(`${MAKE} test -C worker`);

		break;
	}

	case 'coverage':
	{
		replaceVersion();
		executeCmd('jest --coverage');
		executeCmd('open-cli coverage/lcov-report/index.html');

		break;
	}

	case 'release':
	{
		buildTypescript(/* force */ true);
		buildWorker();
		executeCmd('npm run lint');
		executeCmd('npm run test');
		executeCmd(`git commit -am '${version}'`);
		executeCmd(`git tag -a ${version} -m '${version}'`);
		executeCmd(`git push origin v${MAYOR_VERSION} && git push origin --tags`);
		executeCmd('npm publish');

		break;
	}

	case 'install-clang-tools':
	{
		executeCmd('npm ci --prefix worker/scripts');

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
	console.log('npm-scripts.js [INFO] deleteNodeLib()');

	if (!isWindows)
	{
		executeCmd('rm -rf node/lib/');
	}
	else
	{
		// NOTE: This command fails in Windows if the dir doesn't exist.
		executeCmd('rmdir /s /q "node/lib/"', /* exitOnError */ false);
	}
}

function buildTypescript(force = false)
{
	if (!force && fs.existsSync('node/lib/'))
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
			process.exit(1);
		}
	}
}
