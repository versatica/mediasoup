'use strict';

const debug = require('debug')('mediasoup');
const debugerror = require('debug')('mediasoup:ERROR');

debugerror.log = console.error.bind(console);

const Server = require('./lib/Server');

module.exports =
{
	createServer : function(options)
	{
		debug('createServer()');

		return new Server(options);
	}
};
