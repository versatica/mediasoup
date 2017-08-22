'use strict';

const Logger = require('./Logger');
const Server = require('./Server');
const errors = require('./errors');
const webrtc = require('./webrtc');
const PKG = require('../package.json');

const logger = new Logger();

logger.debug('%s version %s', PKG.name, PKG.version);

// Set of Server instances.
const servers = new Set();

module.exports =
{
	/**
	 * Create a Server instance.
	 *
	 * @param {object} [options]
	 * @param {number} [options.numWorkers=HOST_NUM_CORES] - Number of child workers.
	 * @param {string} [options.logLevel='debug'] - Log level. Valid values are
	 * 'debug', 'warn', 'error'.
	 * @param {array} [options.logTags] - Log tags.
	 * @param {string|boolean} [options.rtcIPv4=true] - IPv4 for RTC. Valid
	 * values are a IPv4, `true` (auto-detect) and `false` (disabled).
	 * @param {string|boolean} [options.rtcIPv6=true] - IPv6 for RTC. Valid
	 * values are a IPv6, `true` (auto-detect) and `false` (disabled).
	 * @param {string} [options.rtcAnnouncedIPv4] - Announced IPv4 for RTC. Valid
	 * value is a IPv4.
	 * @param {string} [options.rtcAnnouncedIPv6] - Announced IPv6 for RTC. Valid
	 * value is a IPv6.
	 * @param {number} [options.rtcMinPort=10000] - Minimun RTC port.
	 * @param {number} [options.rtcMaxPort=59999] - Maximum RTC port.
	 * @param {string} [options.dtlsCertificateFile] - Path to DTLS certificate.
	 * @param {string} [options.dtlsPrivateKeyFile] - Path to DTLS private key.
	 *
	 * @return {Server}
	 */
	Server(options)
	{
		logger.debug('Server() [options:%o]', options);

		const server = new Server(options);

		// Store the Server instance and remove it when closed.
		servers.add(server);
		server.on('close', () => servers.delete(server));

		return server;
	},

	/**
	 * Export mediasoup custom errors.
	 */
	errors : errors,

	/**
	 * Export the webrtc module.
	 */
	webrtc : webrtc
};

// On process exit close all the Servers.
process.on('exit', () =>
{
	for (const server of servers)
	{
		server.close();
	}
});

// Log 'unhandledRejection' events.
// NOTE: Those events could be fired due to user code or other running libraries.
process.on('unhandledRejection', (reason) =>
{
	if (reason instanceof Error)
	{
		logger.error(
			'"unhandledRejection" event due to a Promise being rejected with no error handler:\n%s',
			reason.stack);
	}
	else
	{
		logger.error(
			'"unhandledRejection" event due to a Promise being rejected with no error handler [reason:%o]',
			reason);
	}
});
