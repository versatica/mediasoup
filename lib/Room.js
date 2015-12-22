'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Room');
const utils = require('./utils');
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

		// Set of Peer instances
		this._peers = new Set();

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

		// Close every Peer
		this._peers.forEach((peer) => peer.close());

		this.emit('close', error);
	}

	/**
	 * Create a Peer instance
	 * @param  {String}   peerId   Peer identificator
	 * @param  {Object}   options  Peer options
	 * @return {Promise}
	 */
	createPeer(peerId, options)
	{
		logger.debug('createPeer() [peerId:%s, options:%o]', peerId, options);

		if (this._closed)
			return Promise.reject(errors.Closed('room closed'));

		if (!peerId || typeof peerId !== 'string')
			return Promise.reject(errors.TypeError('wrong `peerId` parameter'));

		options = utils.cloneObject(options);

		options.roomId = this._roomId;
		options.peerId = peerId;

		return this._channel.request('createPeer', options)
			.then(() =>
			{
				logger.debug('"createPeer" request succeeded');

				// Create a Peer instance
				var peer = new Peer(options, this._channel);

				// Store the Peer instance and remove it when closed
				this._peers.add(peer);
				peer.on('close', () => this._peers.delete(peer));

				return peer;
			})
			.catch((error) =>
			{
				logger.error('"createPeer" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}
}

module.exports = Room;
