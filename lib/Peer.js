'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Peer');

class Peer extends EventEmitter
{
	constructor(id, data, channel)
	{
		super();

		// TODO: fill it
		let reqData = data;

		this._id = id;

		// Channel instance
		this._channel = channel;

		// Closed flag
		this._closed = false;

		this._channel.request('createPeer', reqData)
			.then((data) =>
			{
				logger.debug('"createPeer" request succeeded [data:%o]', data);
			})
			.catch((error) =>
			{
				logger.error('"createPeer" request failed [status:%s, error:"%s"]', error.status, error.message);

				this.close(error);
			});
	}

	close(error)
	{
		if (this._closed)
		{
			return;
		}
		this._closed = true;

		logger.debug('close()');

		this.emit('close', error);
	}

	get id()
	{
		return this._id;
	}
}

module.exports = Peer;
