const process = require('process');
const os = require('os');
const fs = require('fs');
const { execSync, spawnSync } = require('child_process');
const fetch = require('node-fetch');
const tar = require('tar');

const PKG = JSON.parse(fs.readFileSync('./package.json').toString());
const IS_FREEBSD = os.platform() === 'freebsd';
const IS_WINDOWS = os.platform() === 'win32';
const MAYOR_VERSION = PKG.version.split('.')[0];
const MAKE = process.env.MAKE || (IS_FREEBSD ? 'gmake' : 'make');
const WORKER_RELEASE_DIR = 'worker/out/Release';
const WORKER_RELEASE_BIN = IS_WINDOWS ? 'mediasoup-worker.exe' : 'mediasoup-worker';
const WORKER_RELEASE_BIN_PATH = `${WORKER_RELEASE_DIR}/${WORKER_RELEASE_BIN}`;
const WORKER_PREBUILD_DIR = 'worker/prebuild';
const WORKER_PREBUILD_TAR = `mediasoup-worker-${PKG.version}-${os.platform()}-${os.arch()}.tgz`;
const WORKER_PREBUILD_TAR_PATH = `${WORKER_PREBUILD_DIR}/${WORKER_PREBUILD_TAR}`;

const task = process.argv.slice(2).join(' ');

run();

async function run()
{
	logInfo('run()');

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
				logInfo('MEDIASOUP_WORKER_BIN environment variable given, skipping');

				break;
			}
			else if (process.env.MEDIASOUP_LOCAL_DEV)
			{
				logInfo('MEDIASOUP_LOCAL_DEV environment variable given, building mediasoup-worker locally');

				buildWorker();
				cleanWorkerArtifacts();
			}
			// Attempt to download a prebuilt binary.
			else if (!(await downloadPrebuiltWorker()))
			{
				logInfo(`couldn't fetch any mediasoup-worker prebuilt binary, building it locally`);

				// Fallback to building locally.
				buildWorker();
				cleanWorkerArtifacts();
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
			const watch = new TscWatchClient();

			deleteNodeLib();

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
			if (!process.env.GITHUB_TOKEN)
			{
				logError('missing GITHUB_TOKEN environment variable');
				exitWithError();
			}

			// NOTE: Load dep here since it's a devDependency.
			const { Octokit } = require('@octokit/rest');
			const octokit = new Octokit(
				{
					auth : process.env.GITHUB_TOKEN
				});

			checkRelease();
			executeCmd(`git commit -am '${PKG.version}'`);
			executeCmd(`git tag -a ${PKG.version} -m '${PKG.version}'`);
			executeCmd(`git push origin v${MAYOR_VERSION}`);
			executeCmd(`git push origin '${PKG.version}'`);

			await octokit.repos.createRelease(
				{
					owner    : 'versatica',
					repo     : 'mediasoup',
					name     : PKG.version,
					// eslint-disable-next-line camelcase
					tag_name : PKG.version,
					draft    : false
				});

			executeCmd('npm publish');

			break;
		}

		default:
		{
			logError('unknown task');

			exitWithError();
		}
	}
}

function replaceVersion()
{
	logInfo('replaceVersion()');

	const files =
	[
		'node/lib/index.js',
		'node/lib/index.d.ts',
		'node/lib/Worker.js'
	];

	for (const file of files)
	{
		const text = fs.readFileSync(file, { encoding: 'utf8' });
		const result = text.replace(/__MEDIASOUP_VERSION__/g, PKG.version);

		fs.writeFileSync(file, result, { encoding: 'utf8' });
	}
}

function deleteNodeLib()
{
	if (!fs.existsSync('node/lib'))
	{
		return;
	}

	logInfo('deleteNodeLib()');

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

	logInfo('buildTypescript()');

	deleteNodeLib();
	executeCmd('tsc --project node');
}

function buildWorker()
{
	logInfo('buildWorker()');

	if (IS_WINDOWS)
	{
		if (!fs.existsSync('worker/out/msys/bin/make.exe'))
		{
			installMsysMake();
		}

		const msysPath = `${process.cwd()}\\worker\\out\\msys\\bin`;

		if (!process.env.PATH.includes(msysPath))
		{
			process.env.PATH = `${msysPath};${process.env.PATH}`;
		}
	}

	executeCmd(`${MAKE} -C worker`);
}

function cleanWorkerArtifacts()
{
	logInfo('cleanWorkerArtifacts()');

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
	logInfo('lintNode()');

	executeCmd('eslint -c node/.eslintrc.js --max-warnings 0 node/src node/.eslintrc.js npm-scripts.js worker/scripts/gulpfile.js');
}

function lintWorker()
{
	logInfo('lintWorker()');

	executeCmd(`${MAKE} lint -C worker`);
}

function testNode()
{
	logInfo('testNode()');

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
	logInfo('testWorker()');

	executeCmd(`${MAKE} test -C worker`);
}

function installNodeDeps()
{
	logInfo('installNodeDeps()');

	// Install/update Node deps.
	executeCmd('npm ci --ignore-scripts');
	// Update package-lock.json.
	executeCmd('npm install --package-lock-only --ignore-scripts');
}

function checkRelease()
{
	logInfo('checkRelease()');

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
	logInfo('installMsysMake()');

	let res = spawnSync('where', [ 'python3.exe' ]);

	if (res.status !== 0)
	{
		res = spawnSync('where', [ 'python.exe' ]);

		if (res.status !== 0)
		{
			logError('`installMsysMake() | cannot find Python executable');
			exitWithError();
		}
	}

	executeCmd(`${String(res.stdout).trim()} worker\\scripts\\getmake.py`);
}

function ensureDir(dir)
{
	logInfo(`ensureDir() [dir:${dir}]`);

	if (!fs.existsSync(dir))
	{
		fs.mkdirSync(dir, { recursive: true });
	}
}

async function prebuildWorker()
{
	logInfo('prebuildWorker()');

	ensureDir(WORKER_PREBUILD_DIR);

	return new Promise((resolve, reject) =>
	{
		// Generate a gzip file which just contains mediasoup-worker binary without
		// any folder.
		tar.create(
			{
				cwd  : WORKER_RELEASE_DIR,
				gzip : true
			},
			[ WORKER_RELEASE_BIN ])
			.pipe(fs.createWriteStream(WORKER_PREBUILD_TAR_PATH))
			.on('finish', resolve)
			.on('error', reject);
	});
}

// Returns a Promise resolving to true if a prebuilt mediasoup-worker binary
// was downloaded and uncompressed, false otherwise.
async function downloadPrebuiltWorker()
{
	const releaseBase = process.env.MEDIASOUP_WORKER_DOWNLOAD_BASE || `${PKG.repository.url.replace('.git', '')}/releases/download`;
	const tarUrl = `${releaseBase}/${PKG.version}/${WORKER_PREBUILD_TAR}`;

	logInfo(`downloadPrebuiltWorker() [tarUrl:${tarUrl}]`);

	ensureDir(WORKER_PREBUILD_DIR);

	let res;

	try
	{
		res = await fetch(tarUrl);

		if (res.status === 404)
		{
			logInfo(
				'downloadPrebuiltWorker() | no available prebuilt mediasoup-worker binary for current architecture'
			);

			return false;
		}
		else if (!res.ok)
		{
			logError(
				`downloadPrebuiltWorker() | failed to download prebuilt mediasoup-worker binary: ${res.status} ${res.statusText}`
			);

			return false;
		}
	}
	catch (error)
	{
		logError(
			`downloadPrebuiltWorker() | failed to download prebuilt mediasoup-worker binary: ${error}`
		);

		return false;
	}

	ensureDir(WORKER_RELEASE_DIR);

	return new Promise((resolve) =>
	{
		// Extract mediasoup-worker in the official mediasoup-worker path.
		res.body
			.pipe(tar.extract(
				{
					newer : false,
					cwd   : WORKER_RELEASE_DIR
				}))
			.on('finish', () =>
			{
				logInfo('downloadPrebuiltWorker() | got prebuilt mediasoup-worker binary');

				try
				{
					// Give execution permission to the binary.
					fs.chmodSync(WORKER_RELEASE_BIN_PATH, 0o775);

					resolve(true);
				}
				catch (error)
				{
					logError(
						`downloadPrebuiltWorker() | failed to download prebuilt mediasoup-worker binary: ${error}`
					);

					resolve(false);
				}
			})
			.on('error', (error) =>
			{
				logError(
					`downloadPrebuiltWorker() | failed to uncompress downloaded prebuilt mediasoup-worker binary: ${error}`
				);

				resolve(false);
			});
	});
}

function executeCmd(command, exitOnError = true)
{
	logInfo(`executeCmd(): ${command}`);

	try
	{
		execSync(command, { stdio: [ 'ignore', process.stdout, process.stderr ] });
	}
	catch (error)
	{
		if (exitOnError)
		{
			logError(`executeCmd() failed, exiting: ${error}`);
			exitWithError();
		}
		else
		{
			logInfo(`executeCmd() failed, ignoring: ${error}`);
		}
	}
}

function logInfo(message)
{
	// eslint-disable-next-line no-console
	console.log(`npm-scripts.js \x1b[36m[INFO] [${task}]\x1b\[0m`, message);
}

// eslint-disable-next-line no-unused-vars
function logWarn(message)
{
	// eslint-disable-next-line no-console
	console.warn(`npm-scripts.js \x1b[33m[WARN] [${task}]\x1b\[0m`, message);
}

function logError(message)
{
	// eslint-disable-next-line no-console
	console.error(`npm-scripts.js \x1b[31m[ERROR] [${task}]\x1b\[0m`, message);
}

function exitWithError()
{
	process.exit(1);
}
