import process from 'process';
import os from 'os';
import fs from 'fs';
import path from 'path';
import { execSync, spawnSync } from 'child_process';
import fetch from 'node-fetch';
import tar from 'tar';

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
const GH_OWNER = 'versatica';
const GH_REPO = 'mediasoup';

const task = process.argv.slice(2).join(' ');

run();

async function run()
{
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
			// If the user/app provides us with a custom mediasoup-worker binary then
			// don't do anything.
			if (process.env.MEDIASOUP_WORKER_BIN)
			{
				logInfo('MEDIASOUP_WORKER_BIN environment variable given, skipping');

				break;
			}
			// If MEDIASOUP_LOCAL_DEV is given, or if MEDIASOUP_SKIP_WORKER_PREBUILT_DOWNLOAD
			// env is given, or if mediasoup package is being installed via git+ssh
			// (instead of via npm), and if MEDIASOUP_FORCE_PREBUILT_WORKER_DOWNLOAD env is
			// not set, then skip mediasoup-worker prebuilt download.
			else if (
				(
					process.env.MEDIASOUP_LOCAL_DEV ||
					process.env.MEDIASOUP_SKIP_WORKER_PREBUILT_DOWNLOAD ||
					process.env.npm_package_resolved?.startsWith('git+ssh://')
				) &&
				!process.env.MEDIASOUP_FORCE_WORKER_PREBUILT_DOWNLOAD
			)
			{
				logInfo('skipping mediasoup-worker prebuilt download, building it locally');

				buildWorker();

				if (!process.env.MEDIASOUP_LOCAL_DEV)
				{
					cleanWorkerArtifacts();
				}
			}
			// Attempt to download a prebuilt binary. Fallback to building locally.
			else if (!(await downloadPrebuiltWorker()))
			{
				logInfo(`couldn't fetch any mediasoup-worker prebuilt binary, building it locally`);

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
			deleteNodeLib();
			executeCmd('tsc --project node --watch');

			break;
		}

		case 'worker:build':
		{
			buildWorker();

			break;
		}

		case 'worker:prebuild':
		{
			await prebuildWorker();

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
			let octokit;
			let versionChanges;

			try
			{
				octokit = await getOctokit();
				versionChanges = await getVersionChanges();
			}
			catch (error)
			{
				logError(error.message);

				exitWithError();
			}

			checkRelease();
			executeCmd(`git commit -am '${PKG.version}'`);
			executeCmd(`git tag -a ${PKG.version} -m '${PKG.version}'`);
			executeCmd(`git push origin v${MAYOR_VERSION}`);
			executeCmd(`git push origin '${PKG.version}'`);

			logInfo('creating release in GitHub');

			await octokit.repos.createRelease(
				{
					owner    : GH_OWNER,
					repo     : GH_REPO,
					name     : PKG.version,
					body     : versionChanges,
					// eslint-disable-next-line camelcase
					tag_name : PKG.version,
					draft    : false
				});

			// GitHub mediasoup-worker-prebuild CI action doesn't create mediasoup-worker
			// prebuilt binary for macOS ARM. If this is a macOS ARM machine, do it here
			// and upload it to the release.
			if (os.platform() === 'darwin' && os.arch() === 'arm64')
			{
				await uploadMacArmPrebuiltWorker();
			}

			executeCmd('npm publish');

			break;
		}

		case 'release:upload-mac-arm-prebuilt-worker':
		{
			checkRelease();
			await prebuildWorker();
			await uploadMacArmPrebuiltWorker();

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

	const files = fs.readdirSync('node/lib',
		{
			withFileTypes : true,
			recursive     : true
		});

	for (const file of files)
	{
		if (!file.isFile())
		{
			continue;
		}

		const filePath = path.join('node/lib', file.name);
		const text = fs.readFileSync(filePath, { encoding: 'utf8' });
		const result = text.replace(/__MEDIASOUP_VERSION__/g, PKG.version);

		fs.writeFileSync(filePath, result, { encoding: 'utf8' });
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

	executeCmd('eslint -c node/.eslintrc.js --max-warnings 0 node/src node/.eslintrc.js npm-scripts.mjs worker/scripts/gulpfile.mjs');
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

// Returns a Promise resolving to true if a mediasoup-worker prebuilt binary
// was downloaded and uncompressed, false otherwise.
async function downloadPrebuiltWorker()
{
	const releaseBase =
		process.env.MEDIASOUP_WORKER_PREBUILT_DOWNLOAD_BASE_URL ||
		`${PKG.repository.url.replace(/\.git$/, '')}/releases/download`;

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
				'downloadPrebuiltWorker() | no available mediasoup-worker prebuilt binary for current architecture'
			);

			return false;
		}
		else if (!res.ok)
		{
			logError(
				`downloadPrebuiltWorker() | failed to download mediasoup-worker prebuilt binary: ${res.status} ${res.statusText}`
			);

			return false;
		}
	}
	catch (error)
	{
		logError(
			`downloadPrebuiltWorker() | failed to download mediasoup-worker prebuilt binary: ${error}`
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
				logInfo('downloadPrebuiltWorker() | got mediasoup-worker prebuilt binary');

				try
				{
					// Give execution permission to the binary.
					fs.chmodSync(WORKER_RELEASE_BIN_PATH, 0o775);
				}
				catch (error)
				{
					logWarn(`downloadPrebuiltWorker() | failed to give execution permissions to the mediasoup-worker prebuilt binary: ${error}`);
				}

				// Let's confirm that the fetched mediasoup-worker prebuit binary does
				// run in current host. This is to prevent weird issues related to
				// different versions of libc in the system and so on.
				// So run mediasoup-worker without the required MEDIASOUP_VERSION env and
				// expect exit code 41 (see main.cpp).

				logInfo(
					'downloadPrebuiltWorker() | checking fetched mediasoup-worker prebuilt binary in current host'
				);

				try
				{
					const resolvedBinPath = path.resolve(WORKER_RELEASE_BIN_PATH);

					execSync(
						resolvedBinPath,
						{
							stdio : [ 'ignore', 'ignore', 'ignore' ],
							// Ensure no env is passed to avoid accidents.
							env   : {}
						}
					);
				}
				catch (error)
				{
					if (error.status === 41)
					{
						resolve(true);
					}
					else
					{
						logError(
							`downloadPrebuiltWorker() | fetched mediasoup-worker prebuilt binary fails to run in this host [status:${error.status}]`
						);

						try
						{
							fs.unlinkSync(WORKER_RELEASE_BIN_PATH);
						}
						catch (error2)
						{}

						resolve(false);
					}
				}
			})
			.on('error', (error) =>
			{
				logError(
					`downloadPrebuiltWorker() | failed to uncompress downloaded mediasoup-worker prebuilt binary: ${error}`
				);

				resolve(false);
			});
	});
}

async function uploadMacArmPrebuiltWorker()
{
	if (os.platform() !== 'darwin' || os.arch() !== 'arm64')
	{
		logWarn('uploadMacArmPrebuiltWorker() | invalid platform or architecture');

		return;
	}

	const octokit = await getOctokit();

	logInfo('uploadMacArmPrebuiltWorker() | getting release info');

	const release = await octokit.rest.repos.getReleaseByTag(
		{
			owner : GH_OWNER,
			repo  : GH_REPO,
			tag   : PKG.version
		});

	logInfo('uploadMacArmPrebuiltWorker() | uploading release asset');

	await octokit.rest.repos.uploadReleaseAsset(
		{
			owner      : GH_OWNER,
			repo       : GH_REPO,
			// eslint-disable-next-line camelcase
			release_id : release.data.id,
			name       : WORKER_PREBUILD_TAR,
			data       : fs.readFileSync(WORKER_PREBUILD_TAR_PATH)
		});
}

async function getOctokit()
{
	if (!process.env.GITHUB_TOKEN)
	{
		throw new Error('missing GITHUB_TOKEN environment variable');
	}

	// NOTE: Load dep on demand since it's a devDependency.
	const { Octokit } = await import('@octokit/rest');

	const octokit = new Octokit(
		{
			auth : process.env.GITHUB_TOKEN
		});

	return octokit;
}

async function getVersionChanges()
{
	logInfo('getVersionChanges()');

	// NOTE: Load dep on demand since it's a devDependency.
	const marked = await import('marked');

	const changelog = fs.readFileSync('./CHANGELOG.md').toString();
	const entries = marked.lexer(changelog);

	for (let idx = 0; idx < entries.length; ++idx)
	{
		const entry = entries[idx];

		if (entry.type === 'heading' && entry.text === PKG.version)
		{
			const changes = entries[idx + 1].raw;

			return changes;
		}
	}

	// This should not happen (unless author forgot to update CHANGELOG).
	throw new Error(`no entry found in CHANGELOG.md for version '${PKG.version}'`);
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
	console.log(`npm-scripts \x1b[36m[INFO] [${task}]\x1b\[0m`, message);
}

function logWarn(message)
{
	// eslint-disable-next-line no-console
	console.warn(`npm-scripts \x1b[33m[WARN] [${task}]\x1b\[0m`, message);
}

function logError(message)
{
	// eslint-disable-next-line no-console
	console.error(`npm-scripts \x1b[31m[ERROR] [${task}]\x1b\[0m`, message);
}

function exitWithError()
{
	process.exit(1);
}
