'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Room');
const errors = require('./errors');
const Peer = require('./Peer');

class Room extends EventEmitter
{
	constructor(options, channel)
	{
		logger.debug('constructor() [options:%o]', options);

		super();

		// Room identificator
		this._roomId = options.roomId;

		// Channel instance
		this._channel = channel;

		// Map of Peer instances
		this._peers = new Map();

		// Closed flag
		this._closed = false;

		// TODO: subscribe to channel notifications from the worker
	}

	get closed()
	{
		return this._closed;
	}

	/**
	 * Close the Room
	 */
	close(error, dontSendChannel)
	{
		if (!error)
			logger.debug('close()');
		else
			logger.error('close() [error:%s]', error);

		if (this._closed)
			return;

		this._closed = true;

		// Close every Peer
		for (let peer of this._peers.values())
		{
			peer.close(undefined, true);
		}

		if (!dontSendChannel)
		{
			let data =
			{
				roomId : this._roomId
			};

			// Send Channel request
			this._channel.request('room.close', data)
				.then(() =>
				{
					logger.debug('"room.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"room.close" request failed [status:%s, error:"%s"]', error.status, error.message);
				});
		}

		this.emit('close', error);
	}

	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(errors.Closed('room closed'));

		let data =
		{
			roomId : this._roomId
		};

		return this._channel.request('room.dump', data)
			.then((data) =>
			{
				logger.debug('"room.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"room.dump" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}

	/**
	 * Create a Peer instance
	 * @param  {String} [mandatory]  peerId  Peer identificator
	 * @return {Peer}
	 */
	Peer(peerId)
	{
		logger.debug('Peer() [peerId:%s]', peerId);

		if (this._closed)
			throw errors.Closed('room closed');

		if (!peerId || typeof peerId !== 'string')
			throw errors.TypeError('wrong `peerId` parameter');

		if (this._peers.has(peerId))
			throw new Error(`peer already exists [peerId:${peerId}]`);

		let options =
		{
			roomId : this._roomId,
			peerId : peerId
		};

		// Create a Peer instance
		let peer = new Peer(options, this._channel);

		// Store the Peer instance and remove it when closed
		this._peers.set(peerId, peer);
		peer.on('close', () => this._peers.delete(peerId));

		this._channel.request('room.createPeer', options)
			.then(() => logger.debug('"room.createPeer" request succeeded'))
			.catch((error) =>
			{
				logger.error('"room.createPeer" request failed [status:%s, error:"%s"]', error.status, error.message);

				peer.close(error, true);
			});

		return peer;
	}

	/**
	 * Get Peer
	 * @param  {String} [mandatory] peerId
	 * @return {Peer}
	 */
	getPeer(peerId)
	{
		return this._peers.get(peerId);
	}

	/**
	 * Get an array with all the Peers
	 * @return {Array<Peer>}
	 */
	getPeers()
	{
		return Array.from(this._peers.values());
	}
}

module.exports = Room;
