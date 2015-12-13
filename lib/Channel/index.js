'use strict';

const debug = require('debug')('mediasoup:Channel');
const debugerror = require('debug')('mediasoup:ERROR:Channel');

debugerror.log = console.error.bind(console);

class Channel
{
	constructor(socket)
	{
		debug('constructor()');

		this._socket = socket;
	}

	sendRequest(req)
	{
		debug('sendRequest() [request:%o]', req);

		this._socket.write(JSON.stringify(req));
	}
}

module.exports = Channel;
