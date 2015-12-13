'use strict';

const debug = require('debug')('mediasoup');
const debugerror = require('debug')('mediasoup:ERROR');

debugerror.log = console.error.bind(console);

const process = require('process');

const Server = require('./Server');

// Set of Server instances
var servers = new Set();

module.exports =
{
	createServer : function(options)
	{
		debug('createServer()');

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
