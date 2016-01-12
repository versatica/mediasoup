'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Room');
const utils = require('./utils');
const errors = require('./errors');
const Peer = require('./Peer');

class Room extends EventEmitter
{
	constructor(internal, channel)
	{
		logger.debug('constructor() [internal:%o]', internal);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data
		// - .roomId
		this._internal = internal;

		// Channel instance
		this._channel = channel;

		// Map of Peer instances
		this._peers = new Map();

		// Closed flag
		this._closed = false;

		// Subscribe to notifications
		this._channel.on(this._internal.roomId, (event) =>
		{
			switch (event)
			{
				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
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

		// Remove notification subscriptions
		this._channel.removeAllListeners(this._internal.roomId);

		// Close every Peer
		for (let peer of this._peers.values())
		{
			peer.close(undefined, true);
		}

		if (!dontSendChannel)
		{
			let internal = this._internal;

			// Send Channel request
			this._channel.request('room.close', internal)
				.then(() =>
				{
					logger.debug('"room.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"room.close" request failed: %s', error);
				});
		}

		// TODO: unsubscribe from Channel

		this.emit('close', error);
	}

	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Room closed'));

		let internal = this._internal;

		return this._channel.request('room.dump', internal)
			.then((data) =>
			{
				logger.debug('"room.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"room.dump" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Create a Peer instance
	 * @param  {String} [mandatory]  peerName  Peer identificator
	 * @return {Peer}
	 */
	Peer(peerName)
	{
		logger.debug('Peer() [peerName:%s]', peerName);

		if (this._closed)
			throw new errors.InvalidStateError('Room closed');

		if (!peerName || typeof peerName !== 'string')
			throw new TypeError('`peerName` must be a string');

		if (this._peers.has(peerName))
			throw new Error(`peer already exists [peerName:${peerName}]`);

		let internal =
		{
			roomId   : this._internal.roomId,
			peerId   : utils.randomNumber(),
			peerName : peerName
		};

		// Create a Peer instance
		let peer = new Peer(internal, this._channel);

		// Store the Peer instance and remove it when closed
		this._peers.set(peerName, peer);
		peer.on('close', () => this._peers.delete(peerName));

		this._channel.request('room.createPeer', internal)
			.then(() =>
			{
				logger.debug('"room.createPeer" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"room.createPeer" request failed: %s', error);

				peer.close(error, true);
			});

		return peer;
	}

	/**
	 * Get Peer
	 * @param  {String} [mandatory] peerName
	 * @return {Peer}
	 */
	getPeer(peerName)
	{
		return this._peers.get(peerName);
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
