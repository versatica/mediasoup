const process = require('process');
const os = require('os');
const fs = require('fs');
const { execSync } = require('child_process');
const { version } = require('./package.json');

const isWindows = os.platform() === 'win32';
const task = process.argv.slice(2).join(' ');

const GULP = process.env.GULP || 'gulp';

// mediasoup mayor version.
const MAYOR_VERSION = 3;

// Just for Windows.
let PYTHON;
let MSBUILD;
let MEDIASOUP_BUILDTYPE;
let MEDIASOUP_TEST_TAGS;

if (isWindows)
{
	PYTHON = process.env.PYTHON || 'python';
	MSBUILD = process.env.MSBUILD || 'MSBuild';
	MEDIASOUP_BUILDTYPE = process.env.MEDIASOUP_BUILDTYPE || 'Release';
	MEDIASOUP_TEST_TAGS = process.env.MEDIASOUP_TEST_TAGS || '';
}

// eslint-disable-next-line no-console
console.log(`npm-scripts.js [INFO] running task "${task}"`);

switch (task)
{
	case 'typescript:build':
	{
		if (!isWindows)
		{
			execute('rm -rf lib');
		}
		else
		{
			execute('rmdir /s /q lib');
		}

		execute('tsc');
		taskReplaceVersion();

		break;
	}

	case 'typescript:watch':
	{
		const TscWatchClient = require('tsc-watch/client');

		if (!isWindows)
		{
			execute('rm -rf lib');
		}
		else
		{
			execute('rmdir /s /q lib');
		}

		const watch = new TscWatchClient();

		watch.on('success', taskReplaceVersion);
		watch.start('--pretty');

		break;
	}

	case 'lint:node':
	{
		execute('cross-env MEDIASOUP_NODE_LANGUAGE=typescript eslint -c .eslintrc.js --max-warnings 0 --ext=ts src/');
		execute('cross-env MEDIASOUP_NODE_LANGUAGE=javascript eslint -c .eslintrc.js --max-warnings 0 --ext=js --ignore-pattern \'!.eslintrc.js\' .eslintrc.js gulpfile.js npm-scripts.js test/');

		break;
	}

	case 'lint:worker':
	{
		execute(`${GULP} lint:worker`);

		break;
	}

	case 'format:worker':
	{
		execute(`${GULP} format:worker`);

		break;
	}

	case 'test:node':
	{
		taskReplaceVersion();

		if (!process.env.TEST_FILE)
		{
			execute('jest');
		}
		else
		{
			execute(`jest --testPathPattern ${process.env.TEST_FILE}`);
		}

		break;
	}

	case 'test:worker':
	{
		if (!isWindows)
		{
			execute('make test -C worker');
		}
		else if (!process.env.MEDIASOUP_WORKER_BIN)
		{
			execute(`${PYTHON} ./worker/scripts/configure.py --format=msvs -R mediasoup-worker-test`);
			execute(`${MSBUILD} ./worker/mediasoup-worker.sln /p:Configuration=${MEDIASOUP_BUILDTYPE}`);
			execute(`cd worker && .\\out\\${MEDIASOUP_BUILDTYPE}\\mediasoup-worker-test.exe --invisibles --use-colour=yes ${MEDIASOUP_TEST_TAGS}`);
		}

		break;
	}

	case 'coverage':
	{
		taskReplaceVersion();
		execute('jest --coverage');
		execute('open-cli coverage/lcov-report/index.html');

		break;
	}

	case 'postinstall':
	{
		if (!process.env.MEDIASOUP_WORKER_BIN)
		{
			if (!isWindows)
			{
				execute('make -C worker');
			}
			else
			{
				execute(`${PYTHON} ./worker/scripts/configure.py --format=msvs -R mediasoup-worker`);
				execute(`${MSBUILD} ./worker/mediasoup-worker.sln /p:Configuration=${MEDIASOUP_BUILDTYPE}`);
			}
		}

		break;
	}

	case 'release':
	{
		execute('node npm-scripts.js typescript:build');
		execute('npm run lint');
		execute('npm run test');
		execute(`git commit -am '${version}'`);
		execute(`git tag -a ${version} -m '${version}'`);
		execute(`git push origin v${MAYOR_VERSION} && git push origin --tags`);
		execute('npm publish');

		break;
	}

	default:
	{
		throw new TypeError(`unknown task "${task}"`);
	}
}

function taskReplaceVersion()
{
	const files = [ 'lib/index.js', 'lib/index.d.ts', 'lib/Worker.js' ];

	for (const file of files)
	{
		const text = fs.readFileSync(file, { encoding: 'utf8' });
		const result = text.replace(/__MEDIASOUP_VERSION__/g, version);

		fs.writeFileSync(file, result, { encoding: 'utf8' });
	}
}

function execute(command)
{
	// eslint-disable-next-line no-console
	console.log(`npm-scripts.js [INFO] executing command: ${command}`);

	try
	{
		execSync(command,	{ stdio: [ 'ignore', process.stdout, process.stderr ] });
	}
	catch (error)
	{
		process.exit(1);
	}
}
