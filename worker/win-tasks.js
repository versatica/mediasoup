const exec = require('child_process').exec;
const path = require('path');

const PYTHON = process.env.PYTHON || 'python';
const GULP = path.join(__dirname, '..', 'node_modules', '.bin', 'gulp');
const MSBUILD = process.env.MSBUILD || 'MSBuild';
const MEDIASOUP_BUILDTYPE = process.env.MEDIASOUP_BUILDTYPE || 'Release';

run();

function run() {
    const usage = 'usage:[-h/help/make/test/lint/format]';
    if (process.argv.length < 3) {
        console.error(usage);
        return;
    }
    
    var command = process.argv[2];
    switch (command) {
        case '-h':
        case 'help':
            console.log(usage);
            break;
        case 'make': {
            if (!process.env.MEDIASOUP_WORKER_BIN) {
                var generScript = `${PYTHON} ./worker/scripts/configure.py --format=msvs -R mediasoup-worker`;
                var buildScript = `${MSBUILD} ./worker/mediasoup-worker.sln /p:Configuration=${MEDIASOUP_BUILDTYPE}`;
                execute(`${generScript} && ${buildScript}`)
            }
            break;
        }
        case 'test': {
            if (!process.env.MEDIASOUP_WORKER_BIN) {
                var generScript = `${PYTHON} ./worker/scripts/configure.py --format=msvs -R mediasoup-worker-test`;
                var buildScript = `${MSBUILD} ./worker/mediasoup-worker.sln /p:Configuration=${MEDIASOUP_BUILDTYPE}`;
                // TODO(tonywu) : find a alternative tool to replace LCOV
                execute(`${generScript} && ${buildScript}`)
            }
            break;
        }
        case 'lint': {
            execute(`${GULP} lint:worker`);
            break;
        }
        case 'format': {
            execute(`${GULP} format:worker`);
            break;
        }
        default: {
            console.warn('unknown command');
        }
    }
}

function execute(command) {
    var childProcess = exec(command);
    childProcess.stdout.pipe(process.stdout);
    childProcess.stderr.pipe(process.stderr);
};
