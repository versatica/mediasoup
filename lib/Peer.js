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

	/**
	 * Close the Peer
	 */
	close(error, dontSendChannel)
	{
		logger.debug('close() [error:%s]', error);

		if (this._closed)
			return;

		this._closed = true;

		if (!dontSendChannel)
		{
			let data =
			{
				roomId: this._roomId,
				peerId: this._peerId
			};

			// Send Channel request
			this._channel.request('closePeer', data)
				.then(() =>
				{
					logger.debug('"closePeer" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"closePeer" request failed [status:%s, error:"%s"]', error.status, error.message);
				});
		}

		this.emit('close', error);
	}

	/**
	 * Dump the Room
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		let data =
		{
			roomId: this._roomId,
			peerId: this._peerId
		};

		return this._channel.request('dumpPeer', data)
			.then((data) =>
			{
				logger.debug('"dumpPeer" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"dumpPeer" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}
}

module.exports = Peer;
