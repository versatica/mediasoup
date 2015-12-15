'use strict';

const process = require('process');
const logger = require('./logger')();
const Server = require('./Server');

// Set of Server instances
var servers = new Set();

module.exports =
{
	createServer : function(options)
	{
		logger.debug('createServer()');

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
	servers.forEach(server => server.close());
});
