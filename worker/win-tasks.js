const exec = require('child_process').exec;
const path = require('path');

const PYTHON = process.env.PYTHON || 'python';
const GULP = path.join(__dirname, '..', 'node_modules', '.bin', 'gulp');
const MSBUILD = process.env.MSBUILD || 'MSBuild';
const MEDIASOUP_BUILDTYPE = process.env.MEDIASOUP_BUILDTYPE || 'Release';
const MEDIASOUP_TEST_TAGS = process.env.MEDIASOUP_TEST_TAGS || '';

const usage = 'usage:[-h/help/make/test/lint/format]';

run();

/* eslint-disable no-console */
function run() 
{
	if (process.argv.length < 3) 
	{
		console.error(usage);

		return;
	}

	const command = process.argv[2];

	switch (command) 
	{
		case '-h':
		case 'help':
		{
			console.log(usage);
			break;
		}
		case 'make': 
		{
			if (!process.env.MEDIASOUP_WORKER_BIN) 
			{
				const generScript = `${PYTHON} ./worker/scripts/configure.py --format=msvs -R mediasoup-worker`;
				const buildScript = `${MSBUILD} ./worker/mediasoup-worker.sln /p:Configuration=${MEDIASOUP_BUILDTYPE}`;
				
				execute(`${generScript} && ${buildScript}`);
			}
			break;
		}
		case 'test': 
		{
			if (!process.env.MEDIASOUP_WORKER_BIN) 
			{
				const generScript = `${PYTHON} ./worker/scripts/configure.py --format=msvs -R mediasoup-worker-test`;
				const buildScript = `${MSBUILD} ./worker/mediasoup-worker.sln /p:Configuration=${MEDIASOUP_BUILDTYPE}`;
				const testScript = `cd worker && .\\out\\${MEDIASOUP_BUILDTYPE}\\mediasoup-worker-test.exe --invisibles --use-colour=yes ${MEDIASOUP_TEST_TAGS}`;
				
				execute(`${generScript} && ${buildScript} && ${testScript}`);
			}
			break;
		}
		case 'lint': 
		{
			execute(`${GULP} lint:worker`);
			break;
		}
		case 'format': 
		{
			execute(`${GULP} format:worker`);
			break;
		}
		default: 
		{
			console.warn('unknown command');
		}
	}
}
/* eslint-enable no-console */

function execute(command) 
{
	const childProcess = exec(command);
	
	childProcess.stdout.pipe(process.stdout);
	childProcess.stderr.pipe(process.stderr);
}
