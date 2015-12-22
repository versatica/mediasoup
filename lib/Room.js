'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Room');
const utils = require('./utils');
const Peer = require('./Peer');

class Room extends EventEmitter
{
	constructor(options, channel)
	{
		super();

		let reqData = {};

		this._id = utils.randomNumber();

		// Channel instance
		this._channel = channel;

		// Map of Peer instances
		this._peers = new Map();

		// Closed flag
		this._closed = false;

		// TODO: make data from options
		reqData = options;
		reqData.roomId = this._id;

		this._channel.request('createRoom_', reqData)
			.then((data) =>
			{
				logger.debug('"createRoom" request succeeded [data:%o]', data);
			})
			.catch((error) =>
			{
				logger.error('"createRoom" request failed [status:%s, error:"%s"]', error.status, error.message);

				this.close(error);
			});

		// TODO: subscribe to room notifications from the worker
	}

	close(error)
	{
		if (this._closed)
		{
			return;
		}
		this._closed = true;

		logger.debug('close()');

		// Close every Peer
		for (let peer of this._peers.values())
		{
			peer.close();
		}

		this.emit('close', error);
	}

	createPeer(id, data)
	{
		logger.debug('createPeer() [id:%s, data:%o]', id, data);

		if (this._closed)
		{
			throw new Error('Room closed');
		}

		if (!id || typeof id !== 'string')
		{
			throw new Error('id must be a non-empty string');
		}

		if (data && typeof data !== 'object')
		{
			throw new Error('data must be an object');
		}

		if (this._peers.has(id))
		{
			throw new Error(`Peer with id "${id}" already exists`);
		}

		data = utils.clone(data) || {};
		data.roomId = this._id;

		let peer = new Peer(id, data, this._channel);

		// Store the Peer instance and remove it when closed
		this._peers.set(id, peer);
		peer.on('close', () => this._peers.delete(id));

		return peer;
	}
}

module.exports = Room;
