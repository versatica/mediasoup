const process = require('process');
const path = require('path');
const spawn = require('child_process').spawn;
const Logger = require('./Logger');
const { version } = require('../package.json');
const Worker = require('./Worker');

const logger = new Logger();

let workerPath;
let workerIdx = 0;

// If env MEDIASOUP_WORKER_PATH is given, use it.
if (process.env.MEDIASOUP_WORKER_PATH)
{
	workerPath = process.env.MEDIASOUP_WORKER_PATH;
}
// Otherwise check if env MEDIASOUP_BUILDTYPE is 'Debug'.
else if (process.env.MEDIASOUP_BUILDTYPE === 'Debug')
{
	workerPath = path.join(
		__dirname, '..', 'worker', 'out', 'Debug', 'mediasoup-worker');
}
// Otherwise use 'Release'.
else
{
	workerPath = path.join(
		__dirname, '..', 'worker', 'out', 'Release', 'mediasoup-worker');
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
 * @param {string} [dtlsCertificateFile] - Path to DTLS certificate.
 * @param {string} [dtlsPrivateKeyFile] - Path to DTLS private key.
 *
 * @promise
 * @fulfill {Worker}
 * @reject {TypeError} if wrong settings.
 * @reject {Error} if unexpected error.
 */
exports.createWorker = function(
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

	const workerId = `worker${++workerIdx}`;
	const args = [ workerId ];

	if (logLevel)
		args.push(`--logLevel=${logLevel}`);

	if (Array.isArray(logTags))
	{
		for (const logTag of logTags)
		{
			if (logTag)
				args.push(`--logTag=${logTag}`);
		}
	}

	if (rtcMinPort)
		args.push(`--rtcMinPort=${rtcMinPort}`);

	if (rtcMaxPort)
		args.push(`--rtcMaxPort=${rtcMaxPort}`);

	if (dtlsCertificateFile)
		args.push(`--dtlsCertificateFile=${dtlsCertificateFile}`);

	if (dtlsPrivateKeyFile)
		args.push(`--dtlsPrivateKeyFile=${dtlsPrivateKeyFile}`);

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

	const worker = new Worker({ id: workerId, child });

	return new Promise((resolve, reject) =>
	{
		worker.on('@succeed', () => resolve(worker));
		worker.on('@settingserror', () => reject(new TypeError('wrong settings')));
		worker.on('@failure', () => reject(new Error('unexpected error')));
	});
};
