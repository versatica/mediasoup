'use strict';

const EventEmitter = require('events').EventEmitter;
const logger = require('./logger')('Room');
const utils = require('./utils');
const errors = require('./errors');
const Peer = require('./Peer');

const KINDS = [ 'audio', 'video', 'depth' ];

class Room extends EventEmitter
{
	constructor(internal, data, channel)
	{
		logger.debug('constructor() [internal:%o, data:%o]', internal, data);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data.
		// - .roomId
		this._internal = internal;

		// Channel instance.
		this._channel = channel;

		// RTP capabilities splitted into supported kinds.
		this._capabilities = {};

		// Map of Peer instances indexed by `peerName`.
		this._peers = new Map();

		// Closed flag.
		this._closed = false;

		// Subscribe to notifications.
		this._channel.on(this._internal.roomId, (event) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, true);
					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});

		// Set room's capabilities.
		this._setCapabilities(data.capabilities);
	}

	get closed()
	{
		return this._closed;
	}

	/**
	 * Get an array with all the Peers.
	 *
	 * @return {Array<Peer>}
	 */
	get peers()
	{
		return Array.from(this._peers.values());
	}

	/**
	 * Close the Room.
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

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.roomId);

		// Close every Peer.
		for (let peer of this._peers.values())
		{
			peer.close(undefined, true);
		}

		if (!dontSendChannel)
		{
			// Send Channel request.
			this._channel.request('room.close', this._internal)
				.then(() =>
				{
					logger.debug('"room.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"room.close" request failed: %s', error);
				});
		}

		this.emit('close', error);
	}

	/**
	 * Dump the Room.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Room closed'));

		return this._channel.request('room.dump', this._internal)
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
	 * Get room's capabilities per kind.
	 *
	 * @return {RtpCapabilities}
	 */
	getCapabilities(kind)
	{
		logger.debug('getCapabilities() [kind:%s]', kind);

		// Ensure `kind` is 'audio' / 'video' / 'depth'.
		if (KINDS.indexOf(kind) === -1)
			throw new TypeError(`unsupported kind: ${kind}`);

		return this._capabilities[kind];
	}

	/**
	 * Create a Peer instance.
	 *
	 * @param {String} peerName - Peer identificator.
	 *
	 * @return {Peer}
	 */
	Peer(peerName)
	{
		logger.debug('Peer() [peerName:"%s]', peerName);

		if (this._closed)
			throw new errors.InvalidStateError('Room closed');

		if (!peerName || typeof peerName !== 'string')
			throw new TypeError('peerName must be a string');

		if (this._peers.has(peerName))
			throw new Error(`peer already exists [peerName:"${peerName}"]`);

		let peerId = utils.randomNumber();
		let internal =
		{
			roomId   : this._internal.roomId,
			peerId   : peerId,
			peerName : peerName
		};
		let sandbox =
		{
			getPeer : this.getPeer.bind(this)
		};

		// Create a Peer instance.
		let peer = new Peer(internal, this._channel, sandbox);

		// Store the Peer instance and remove it when closed.
		this._peers.set(peerName, peer);
		peer.on('close', () =>
		{
			this._peers.delete(peerName);
		});

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
	 * Get Peer.
	 *
	 * @param {String} peerName
	 *
	 * @return {Peer}
	 */
	getPeer(peerName)
	{
		return this._peers.get(peerName);
	}

	_setCapabilities(capabilities)
	{
		for (let kind of KINDS)
		{
			this._capabilities[kind] =
			{
				codecs           : [],
				headerExtensions : [],
				fecMechanisms    : []
			};

			let kindCapabilities = this._capabilities[kind];

			for (let codec of capabilities.codecs)
			{
				if (codec.kind === kind)
				{
					kindCapabilities.codecs.push(codec);
				}
				else if (codec.kind === '')
				{
					let clonedCodec = utils.cloneObject(codec);

					clonedCodec.kind = kind;
					kindCapabilities.codecs.push(clonedCodec);
				}
			}

			// Ignore if there are no media codecs of this kind.
			if (kindCapabilities.codecs.length > 0)
			{
				for (let headerExtension of capabilities.headerExtensions)
				{
					if (headerExtension.kind === kind)
					{
						kindCapabilities.headerExtensions.push(headerExtension);
					}
					else if (headerExtension.kind === '')
					{
						let clonedHeaderExtension = utils.cloneObject(headerExtension);

						clonedHeaderExtension.kind = kind;
						kindCapabilities.headerExtensions.push(clonedHeaderExtension);
					}
				}

				kindCapabilities.fecMechanisms = capabilities.fecMechanisms;
			}
		}
	}
}

module.exports = Room;
