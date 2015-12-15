'use strict';

const EventEmitter = require('events').EventEmitter;
const logger = require('./logger')('Room');

const STATUS =
{
	INIT   : Symbol('init'),
	OPEN   : Symbol('open'),
	CLOSED : Symbol('closed')
};

class Room extends EventEmitter
{
	constructor(options, channel)
	{
		super();

		this._status = STATUS.INIT;

		// Channel instance
		this._channel = channel;

		let data =
		{
			foo   : 'lalala',
			bar   : 1234
		};

		this._channel.sendRequest('createRoom', data);
	}

	close()
	{
		logger.debug('close()');

		this._status = STATUS.CLOSED;

		this.emit('close');
	}
}

module.exports = Room;
