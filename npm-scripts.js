const process = require('process');
const os = require('os');
const fs = require('fs');
const { execSync } = require('child_process');
const { version } = require('./package.json');

const isFreeBSD = os.platform() === 'freebsd';
const isWindows = os.platform() === 'win32';
const task = process.argv.slice(2).join(' ');

// mediasoup mayor version.
const MAYOR_VERSION = 3;

let MAKE;

if (isFreeBSD)
{
	MAKE = process.env.MAKE || 'gmake';
}
else
{
	MAKE = process.env.MAKE || 'make';
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

	case 'worker:build':
	{
		if (!process.env.MEDIASOUP_WORKER_BIN)
		{
			execute(`${MAKE} -C worker`);
		}

		break;
	}

	case 'lint:node':
	{
		execute('cross-env MEDIASOUP_NODE_LANGUAGE=typescript eslint -c .eslintrc.js --max-warnings 0 --ext=ts src/');
		execute('cross-env MEDIASOUP_NODE_LANGUAGE=javascript eslint -c .eslintrc.js --max-warnings 0 --ext=js --ignore-pattern \'!.eslintrc.js\' .eslintrc.js npm-scripts.js test/ worker/scripts/gulpfile.js');

		break;
	}

	case 'lint:worker':
	{
		execute(`${MAKE} lint -C worker`);

		break;
	}

	case 'format:worker':
	{
		execute(`${MAKE} format -C worker`);

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
		execute(`${MAKE} test -C worker`);
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
		execute('node npm-scripts.js worker:build');
		execute(`${MAKE} clean-pip -C worker`);
		execute(`${MAKE} clean-subprojects -C worker`);

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

	case 'install-clang-tools':
	{
		execute('npm install --prefix worker/scripts');

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
