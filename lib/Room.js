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
		this._channel.sendRequest('createRoom', data)
			.then(data =>
			{
				logger.debug('"createRoom" command succeeded: %s', data); // TODO: TMP
				this._status = STATUS.OPEN;
			})
			.catch(error =>
			{
				logger.error('"createRoom" command failed: %s', error);

				// TODO: enable
				// this.close(error);
			});
		// }, 50);
	}

	close(error)
	{
		logger.debug('close()');

		this._status = STATUS.CLOSED;

		this.emit('close', error);
	}
}

module.exports = Room;
