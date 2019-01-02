const process = require('process');
const path = require('path');
const { spawn } = require('child_process');
const { version } = require('../package.json');
const Logger = require('./Logger');
const Worker = require('./Worker');

const logger = new Logger();

let workerPath;

// If env MEDIASOUP_WORKER_PATH is given, use it.
if (process.env.MEDIASOUP_WORKER_PATH)
{
	workerPath = process.env.MEDIASOUP_WORKER_PATH;
}
// Otherwise check if env MEDIASOUP_BUILDTYPE is 'Debug'.
else if (process.env.MEDIASOUP_BUILDTYPE === 'Debug')
{
	workerPath =
		path.join(__dirname, '..', 'worker', 'out', 'Debug', 'mediasoup-worker');
}
// Otherwise use 'Release'.
else
{
	workerPath =
		path.join(__dirname, '..', 'worker', 'out', 'Release', 'mediasoup-worker');
}

/**
 * mediasoup version.
 *
 * @type {String}
 */
exports.version = version;

/**
 * Create a worker.
 *
 * @param {String} [logLevel] - 'debug'/'warn'/'error'/'none' (defaults to 'none').
 * @param {Array<String>} [logTags] - Log tags.
 * @param {Number} [rtcMinPort] - Defaults to 10000.
 * @param {Number} [rtcMaxPort] - Defaults to 59999.
 * @param {String} [dtlsCertificateFile] - Path to DTLS certificate.
 * @param {String} [dtlsPrivateKeyFile] - Path to DTLS private key.
 *
 * @async
 * @returns {Worker}
 * @throws {TypeError} if wrong settings.
 * @throws {Error} if unexpected error.
 */
exports.createWorker = async function(
	{
		logLevel,
		logTags,
		rtcMinPort,
		rtcMaxPort,
		dtlsCertificateFile,
		dtlsPrivateKeyFile
	} = {}
)
{
	logger.debug('createWorker()');

	const args = [];

	if (typeof logLevel === 'string' && logLevel)
		args.push(`--logLevel=${logLevel}`);

	for (const logTag of (Array.isArray(logTags) ? logTags : []))
	{
		if (typeof logTag === 'string' && logTag)
			args.push(`--logTag=${logTag}`);
	}

	if (typeof rtcMinPort === 'number')
		args.push(`--rtcMinPort=${rtcMinPort}`);

	if (typeof rtcMaxPort === 'number')
		args.push(`--rtcMaxPort=${rtcMaxPort}`);

	if (typeof dtlsCertificateFile === 'string' && dtlsCertificateFile)
		args.push(`--dtlsCertificateFile=${dtlsCertificateFile}`);

	if (typeof dtlsPrivateKeyFile === 'string' && dtlsPrivateKeyFile)
		args.push(`--dtlsPrivateKeyFile=${dtlsPrivateKeyFile}`);

	logger.debug('createWorker() | spawing worker process: %s %s', workerPath, args);

	const child = spawn(
		// command
		workerPath,
		// args
		args,
		// options
		{
			detached : false,

			/*
			 * fd 0 (stdin)   : Just ignore it.
			 * fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
			 * fd 2 (stderr)  : Same as stdout.
			 * fd 3 (channel) : Channel fd.
			 */
			stdio : [ 'ignore', 'pipe', 'pipe', 'pipe' ]
		});

	const worker = new Worker({ child });

	return new Promise((resolve, reject) =>
	{
		worker.on('@success', () => resolve(worker));
		worker.on('@failure', reject);
	});
};
