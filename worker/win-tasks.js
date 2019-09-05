'use strict';

const exec = require('child_process').exec;
const path = require('path');

const PYTHON = process.env.PYTHON || 'python';
const GULP = path.join(__dirname, '..', 'node_modules', '.bin', 'gulp');
const MSBUILD = process.env.MSBUILD || 'MSBuild';
const MEDIASOUP_BUILDTYPE = process.env.MEDIASOUP_BUILDTYPE || 'Release';
const DEL = process.env.DEL || 'del';
const RD = process.env.RM || 'rd';

run();

function run() {
    const usage = 'usage:[-h/help/make/test/fuzzer/lint/format/bear/tidy/clean/clean-all/docker-build/docker-run]';
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
        case 'fuzzer': {
            var generScript = `${PYTHON} ./worker/scripts/configure.py --format=msvs -R mediasoup-worker-fuzzer`;
            var buildScript = `${MSBUILD} ./worker/mediasoup-worker.sln /p:Configuration=${MEDIASOUP_BUILDTYPE}`;
            execute(`${generScript} && ${buildScript}`)
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
        case 'bear': {
            console.warn('not support yet.');
            break;
        }
        case 'tidy': {
            console.warn('not support yet.');
            break;
        }
        case 'clean': {
            execute(`${DEL} /s /q .\\worker\\Debug\\`);
            execute(`${DEL} /s /q .\\worker\\Release\\`);

            execute(`${DEL} /s /q .\\worker\\out\\Debug`);
            execute(`${DEL} /s /q .\\worker\\out\\Release`);

            execute(`${DEL} /s /q .\\worker\\deps\\getopt\\Debug`);
            execute(`${DEL} /s /q .\\worker\\deps\\getopt\\Release`);

            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\Debug`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\Release`);

            execute(`${DEL} /s /q .\\worker\\deps\\libuv\\Debug`);
            execute(`${DEL} /s /q .\\worker\\deps\\libuv\\Release`);

            execute(`${DEL} /s /q .\\worker\\deps\\netstring\\Debug`);
            execute(`${DEL} /s /q .\\worker\\deps\\netstring\\Release`);

            execute(`${DEL} /s /q .\\worker\\deps\\openssl\\Debug`);
            execute(`${DEL} /s /q .\\worker\\deps\\openssl\\Release`);

            execute(`${DEL} /s /q .\\worker\\deps\\usrsctp\\Debug`);
            execute(`${DEL} /s /q .\\worker\\deps\\usrsctp\\Release`);
            break;
        }
        case 'clean-all': {
            execute(`${RD} /s /q .\\worker\\out\\`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker.sln`);

            execute(`${RD} /s /q .\\worker\\Debug`);
            execute(`${RD} /s /q .\\worker\\Release\\`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker.vcxproj.user`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker-test.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker-test.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker-test.vcxproj.user`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker-fuzzer.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker-fuzzer.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\mediasoup-worker-fuzzer.vcxproj.user`);

            execute(`${RD} /s /q .\\worker\\deps\\getopt\\Debug`);
            execute(`${RD} /s /q .\\worker\\deps\\getopt\\Release`);
            execute(`${DEL} /s /q .\\worker\\deps\\getopt\\*.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\getopt\\*.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\getopt\\*.vcxproj.user`);

            execute(`${RD} /s /q .\\worker\\deps\\libsrtp\\Debug`);
            execute(`${RD} /s /q .\\worker\\deps\\libsrtp\\Release`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\libsrtp.sln`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\libsrtp.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\libsrtp.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\libsrtp.vcxproj.user`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\rdbx_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\rdbx_driver.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\replay_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\replay_driver.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\roc_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\roc_driver.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\rtpw.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\rtpw.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_driver.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_runtest.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_runtest.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_aes_calc.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_aes_calc.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_cipher_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_cipher_driver.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_datatypes_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_datatypes_driver.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_env.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_env.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_kernel_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_kernel_driver.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_rand_gen.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_rand_gen.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_rand_gen_soak.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_rand_gen_soak.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_sha1_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_sha1_driver.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_stat_driver.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libsrtp\\srtp_test_stat_driver.vcxproj.filters`);

            execute(`${RD} /s /q .\\worker\\deps\\libuv\\Debug`);
            execute(`${RD} /s /q .\\worker\\deps\\libuv\\Release`);
            execute(`${DEL} /s /q .\\worker\\deps\\libuv\\*.sln`);
            execute(`${DEL} /s /q .\\worker\\deps\\libuv\\*.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\libuv\\*.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\libuv\\*.vcxproj.user`);

            execute(`${RD} /s /q .\\worker\\deps\\netstring\\Debug`);
            execute(`${RD} /s /q .\\worker\\deps\\netstring\\Release`);
            execute(`${DEL} /s /q .\\worker\\deps\\netstring\\*.sln`);
            execute(`${DEL} /s /q .\\worker\\deps\\netstring\\*.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\netstring\\*.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\netstring\\*.vcxproj.user`);

            execute(`${RD} /s /q .\\worker\\deps\\openssl\\Debug`);
            execute(`${RD} /s /q .\\worker\\deps\\openssl\\Release`);
            execute(`${DEL} /s /q .\\worker\\deps\\openssl\\*.sln`);
            execute(`${DEL} /s /q .\\worker\\deps\\openssl\\*.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\openssl\\*.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\openssl\\*.vcxproj.user`);

            execute(`${RD} /s /q .\\worker\\deps\\usrsctp\\Debug`);
            execute(`${RD} /s /q .\\worker\\deps\\usrsctp\\Release`);
            execute(`${DEL} /s /q .\\worker\\deps\\usrsctp\\*.sln`);
            execute(`${DEL} /s /q .\\worker\\deps\\usrsctp\\*.vcxproj`);
            execute(`${DEL} /s /q .\\worker\\deps\\usrsctp\\*.vcxproj.filters`);
            execute(`${DEL} /s /q .\\worker\\deps\\usrsctp\\*.vcxproj.user`);
            break;
        }
        case 'docker-build': {
            console.warn('not support yet.');
            break;
        }
        case 'docker-run': {
            console.warn('not support yet.');
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
