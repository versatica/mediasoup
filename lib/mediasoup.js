'use strict';

const process = require('process');

const logger = require('./logger')();
const utils = require('./utils');
const Server = require('./Server');
const errors = require('./errors');
const extra = require('./extra');

const PKG = require('../package.json');

// Set of Server instances.
let servers = new Set();

module.exports =
{
	/**
	 * Create a Server instance.
	 *
	 * @param {Object} [options]
	 * @param {number} [options.numWorkers=HOST_NUM_CORES] - Number of child workers.
	 * @param {string} [options.logLevel='debug'] - Log level. Valid values are
	 * 'debug', 'warn', 'error'.
	 * @param {string|boolean} [options.rtcListenIPv4=true] - IPv4 for RTC. Valid
	 * values are a IPv4, `true` (auto-detect) and `false` (disabled).
	 * @param {string|boolean} [options.rtcListenIPv6=true] - IPv6 for RTC. Valid
	 * values are a IPv6, `true` (auto-detect) and `false` (disabled).
	 * @param {number} [options.rtcMinPort=10000] - Minimun RTC port.
	 * @param {number} [options.rtcMaxPort=59999] - Maximum RTC port.
	 * @param {string} [options.dtlsCertificateFile] - Path to DTLS certificate.
	 * @param {string} [options.dtlsPrivateKeyFile] - Path to DTLS private key.
	 *
	 * @return {Server}
	 */
	Server: function(options)
	{
		logger.debug('Server() [options:%o]', options);

		options = utils.cloneObject(options);

		let server = new Server(options);

		// Store the Server instance and remove it when closed.
		servers.add(server);
		server.on('close', () => servers.delete(server));

		return server;
	},

	/**
	 * Export mediasoup custom errors.
	 */
	errors: errors,

	/**
	 * Export the extra module.
	 */
	extra: extra
};

// On process exit close all the Servers.
process.on('exit', () =>
{
	servers.forEach((server) => server.close());
});

// Log 'unhandledRejection' events.
// NOTE: Those events could be fired due to user code or other running libraries.
process.on('unhandledRejection', (reason) =>
{
	if (reason instanceof Error)
		logger.warn('"unhandledRejection" event due to a Promise being rejected with no error handler:\n%s', reason.stack);
	else
		logger.warn('"unhandledRejection" event due to a Promise being rejected with no error handler [reason:%o]', reason);
});

logger.debug('%s version %s', PKG.name, PKG.version);
