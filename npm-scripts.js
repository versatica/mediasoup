/* eslint-disable no-console */

const process = require('process');
const os = require('os');
const fs = require('fs');
const { execSync, spawnSync } = require('child_process');
const fetch = require('node-fetch');
const tar = require('tar');
const { version, repository } = require('./package.json');

const IS_FREEBSD = os.platform() === 'freebsd';
const IS_WINDOWS = os.platform() === 'win32';
// mediasoup mayor version.
const MAYOR_VERSION = version.split('.')[0];
// make command to use.
const MAKE = process.env.MAKE || (IS_FREEBSD ? 'gmake' : 'make');
const WORKER_BIN_PATH = 'worker/out/Release/mediasoup-worker';
// Prebuilt package related constants.
const WORKER_PREBUILD_DIR = 'worker/prebuild';
const WORKER_PREBUILD_TAR = `mediasoup-worker-${version}-${os.platform()}-${os.arch()}.tgz`;
const WORKER_PREBUILD_TAR_PATH =`${WORKER_PREBUILD_DIR}/${WORKER_PREBUILD_TAR}`;

const task = process.argv.slice(2).join(' ');

run(task);

// eslint-disable-next-line no-shadow
async function run(task)
{
	console.log(`npm-scripts.js [INFO] run() [task:${task}]`);

	switch (task)
	{
		// As per NPM documentation (https://docs.npmjs.com/cli/v9/using-npm/scripts)
		// `prepare` script:
		//
		// - Runs BEFORE the package is packed, i.e. during `npm publish` and
		//   `npm pack`.
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
			if (process.env.MEDIASOUP_WORKER_BIN)
			{
				console.log('npm-scripts.js [INFO] MEDIASOUP_WORKER_BIN environment variable given, skipping "postinstall" task');

				break;
			}

			// Attempt to download a prebuilt binary.
			try
			{
				await downloadPrebuiltWorker();
			}
			catch (error)
			{
				console.warn(`npm-scripts.js [WARN] failed to download prebuilt mediasoup-worker binary, building it locally instead: ${error}`);

				// Fallback to building locally.
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
			// NOTE: Load dep here since it's a devDependency.
			const { TscWatchClient } = require('tsc-watch/client');

			deleteNodeLib();

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

		case 'worker:prebuild':
		{
			prebuildWorker();

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

	if (!IS_WINDOWS)
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

	if (IS_WINDOWS)
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

	if (IS_WINDOWS)
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

function installMsysMake()
{
	console.log('npm-scripts.js [INFO] installMsysMake()');

	let res = spawnSync('where', [ 'python3.exe' ]);

	if (res.status !== 0)
	{
		res = spawnSync('where', [ 'python.exe' ]);

		if (res.status !== 0)
		{
			console.error('`npm-scripts.js [ERROR] installMsysMake() cannot find Python executable');

			process.exit(1);
		}
	}

	executeCmd(`${String(res.stdout).trim()} worker\\scripts\\getmake.py`);
}

function ensureDir(dir)
{
	console.log(`npm-scripts.js [INFO] ensureDir() [dir:${dir}]`);

	if (!fs.existsSync(dir))
	{
		fs.mkdirSync(dir, { recursive: true });
	}
}

async function prebuildWorker()
{
	console.log('npm-scripts.js [INFO] prebuildWorker()');

	ensureDir(WORKER_PREBUILD_DIR);

	return new Promise((resolve, reject) =>
	{
		tar.create({ gzip: true }, [ WORKER_BIN_PATH ])
			.pipe(fs.createWriteStream(WORKER_PREBUILD_TAR_PATH))
			.on('finish', resolve)
			.on('error', reject);
	});
}

async function downloadPrebuiltWorker()
{
	const releaseBase = process.env.MEDIASOUP_WORKER_DOWNLOAD_BASE || `${repository.url.replace('.git', '')}/releases/download`;
	const tarUrl = `${releaseBase}/${version}/${WORKER_PREBUILD_TAR}`;

	console.log(`npm-scripts.js [INFO] downloadPrebuiltWorker() [tarUrl:${tarUrl}]`);

	ensureDir(WORKER_PREBUILD_DIR);

	const res = await fetch(tarUrl);

	return new Promise((resolve, reject) =>
	{
		res.body
			.pipe(tar.x({ strip: 1, cwd: WORKER_PREBUILD_DIR }))
			.on('finish', resolve)
			.on('error', reject);
	});
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

/* eslint-enable no-console */
