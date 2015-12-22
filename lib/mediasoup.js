'use strict';

const process = require('process');

const logger = require('./logger')();
const utils = require('./utils');
const Server = require('./Server');

// Set of Server instances
var servers = new Set();

module.exports =
{
	/**
	 * Create a Server instance
	 * @param  {Object} options  Server options
	 * @return {Server}
	 */
	Server: function(options)
	{
		logger.debug('Server() [options:%o]', options);

		options = utils.cloneObject(options);

		var server = new Server(options);

		// Store the Server instance and remove it when closed
		servers.add(server);
		server.once('close', () => servers.delete(server));

		return server;
	}
};

// On process exit close all the Servers
process.on('exit', () =>
{
	servers.forEach((server) => server.close());
});
