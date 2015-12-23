'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Peer');

class Peer extends EventEmitter
{
	constructor(options, channel)
	{
		logger.debug('constructor() [options:%o]', options);

		super();

		// Room identificator
		this._roomId = options.roomId;

		// Peer identificator
		this._peerId = options.peerId;

		// Channel instance
		this._channel = channel;

		// Closed flag
		this._closed = false;

		// TODO: subscribe to channel notifications from the worker
	}

	close(error)
	{
		logger.debug('close() [error:%s]', error);

		if (this._closed)
			return;

		this._closed = true;

		let request =
		{
			roomId: this._roomId,
			peerId: this._peerId
		};

		// Send Channel request
		this._channel.request('closePeer', request)
			.then(() =>
			{
				logger.debug('"closePeer" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"closePeer" request failed [status:%s, error:"%s"]', error.status, error.message);
			});

		this.emit('close', error);
	}
}

module.exports = Peer;
