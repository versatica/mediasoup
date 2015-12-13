'use strict';

const debug = require('debug')('mediasoup:Room');
const debugerror = require('debug')('mediasoup:ERROR:Room');

debugerror.log = console.error.bind(console);

const EventEmitter = require('events').EventEmitter;

class Room extends EventEmitter
{
	constructor(options, channel)
	{
		super();

		this._channel = channel;

		let req =
		{
			type   : 'request',
			id     : 12345678,
			method : 'createRoom'
		};

		this._channel.sendRequest(req);
	}

	close()
	{
		debug('close()');

		this.emit('close');
	}
}

module.exports = Room;
