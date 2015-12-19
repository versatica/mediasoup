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
			foo : 'lalala',
			baz : 4321,
			bar : 'œæ€😤😮😱Ω∑©'
		};

		// setInterval(() => {
		this._channel.request('createRoom', data)
			.then(() =>
			{
				logger.debug('"createRoom" command succeeded');

				this._status = STATUS.OPEN;

				this.emit('open');
			})
			.catch((error) =>
			{
				logger.error('"createRoom" command failed [error:%s]', error);

				this.close(error);
			});
		// }, 20);
	}

	close(error)
	{
		logger.debug('close()');

		this._status = STATUS.CLOSED;

		this.emit('close', error);
	}
}

module.exports = Room;
