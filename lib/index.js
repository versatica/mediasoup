const { version } = require('../package.json');
const Logger = require('./Logger');
const Worker = require('./Worker');

const logger = new Logger();

/**
 * Expose mediasoup version.
 *
 * @type {String}
 */
exports.version = version;

/**
 * Create a Worker.
 *
 * @param {String} [logLevel='error'] - 'debug'/'warn'/'error'/'none'.
 * @param {Array<String>} [logTags] - Log tags.
 * @param {Number} [rtcMinPort=10000] - Minimum port for ICE/DTLS/RTP/RTCP.
 * @param {Number} [rtcMaxPort=59999] - Maximum port for ICE/DTLS/RTP/RTCP.
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
		logLevel = 'error',
		logTags,
		rtcMinPort = 10000,
		rtcMaxPort = 59999,
		dtlsCertificateFile,
		dtlsPrivateKeyFile
	} = {}
)
{
	logger.debug('createWorker()');

	const worker = new Worker(
		{
			logLevel,
			logTags,
			rtcMinPort,
			rtcMaxPort,
			dtlsCertificateFile,
			dtlsPrivateKeyFile
		});

	return new Promise((resolve, reject) =>
	{
		worker.on('@success', () => resolve(worker));
		worker.on('@failure', reject);
	});
};
