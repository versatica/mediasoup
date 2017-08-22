'use strict';

const EventEmitter = require('events').EventEmitter;
const Logger = require('./Logger');
const utils = require('./utils');
const errors = require('./errors');
const Transport = require('./Transport');
const RtpReceiver = require('./RtpReceiver');
const RtpSender = require('./RtpSender');

const KINDS = [ 'audio', 'video', 'depth' ];

const logger = new Logger('Peer');

class Peer extends EventEmitter
{
	constructor(internal, channel, sandbox)
	{
		logger.debug('constructor() [internal:%o]', internal);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data.
		// - .roomId
		// - .peerId
		// - .peerName
		this._internal = internal;

		// Store internal sandbox stuff.
		// - .getPeerByName()
		// - .getRtpReceiverById()
		this._sandbox = sandbox;

		// Peer data.
		// - .capabilities
		this._data = {};

		// Channel instance.
		this._channel = channel;

		// Set of Transport instances.
		this._transports = new Set();

		// Set of RtpReceiver instances.
		this._rtpReceivers = new Set();

		// Set of RtpSender instances.
		this._rtpSenders = new Set();

		// Closed flag.
		this._closed = false;

		// Subscribe to notifications.
		this._channel.on(this._internal.peerId, (event, data) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, true);
					break;
				}

				case 'newrtpsender':
				{
					const associatedRtpReceiver = this._sandbox.getRtpReceiverById(
						data.associatedRtpReceiverId);

					const internal2 =
					{
						roomId      : this._internal.roomId,
						peerId      : this._internal.peerId,
						rtpSenderId : data.rtpSenderId,
						transportId : null
					};

					const data2 =
					{
						kind                  : data.kind,
						peer                  : this,
						rtpParameters         : data.rtpParameters,
						associatedRtpReceiver : associatedRtpReceiver,
						active                : data.active
					};

					// Create a RtpSender instance.
					const rtpSender = new RtpSender(internal2, data2, this._channel);

					// Store the RtpSender instance and remove it when closed.
					this._rtpSenders.add(rtpSender);
					rtpSender.on('close', () => this._rtpSenders.delete(rtpSender));

					this.emit('newrtpsender', rtpSender);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}

	get id()
	{
		return this._internal.peerId;
	}

	get closed()
	{
		return this._closed;
	}

	get name()
	{
		return this._internal.peerName;
	}

	get capabilities()
	{
		return this._data.capabilities;
	}

	/**
	 * Get an array with all the Transports.
	 *
	 * @return {Array<Transports>}
	 */
	get transports()
	{
		return Array.from(this._transports);
	}

	/**
	 * Get an array with all the RtpReceivers.
	 *
	 * @return {Array<RtpReceivers>}
	 */
	get rtpReceivers()
	{
		return Array.from(this._rtpReceivers);
	}

	/**
	 * Get an array with all the RtpSenders.
	 *
	 * @return {Array<RtpSenders>}
	 */
	get rtpSenders()
	{
		return Array.from(this._rtpSenders);
	}

	/**
	 * Close the Peer.
	 */
	close(error, dontSendChannel)
	{
		if (this._closed)
			return;

		this._closed = true;

		if (!error)
			logger.debug('close()');
		else
			logger.error('close() [error:%s]', error);

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.peerId);

		// Close every Transport.
		for (const transport of this._transports)
		{
			transport.close(undefined, true);
		}

		// Close every RtpReceiver.
		for (const rtpReceiver of this._rtpReceivers)
		{
			rtpReceiver.close(undefined, true);
		}

		// Close every RtpSender.
		for (const rtpSender of this._rtpSenders)
		{
			rtpSender.close();
		}

		if (!dontSendChannel)
		{
			// Send Channel request.
			this._channel.request('peer.close', this._internal)
				.then(() =>
				{
					logger.debug('"peer.close" request succeeded');
				})
				.catch((error2) =>
				{
					logger.error('"peer.close" request failed: %s', error2);
				});
		}

		this.emit('close', error);
	}

	/**
	 * Dump the Peer.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		return this._channel.request('peer.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"peer.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"peer.dump" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Set peer capabilities.
	 *
	 * @param {RtpCapabilities} [capabilities]
	 *
	 * @return {Promise} Resolves to the effective capabilities.
	 */
	setCapabilities(capabilities)
	{
		logger.debug('setCapabilities() [capabilities:%o]', capabilities);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		return this._channel.request('peer.setCapabilities', this._internal, capabilities)
			.then((effectiveCapabilities) =>
			{
				logger.debug('"peer.setCapabilities" request succeeded');

				// Set peer's effective capabilities.
				this._data.capabilities = effectiveCapabilities;

				this.emit('capabilities', effectiveCapabilities);

				return effectiveCapabilities;
			})
			.catch((error) =>
			{
				logger.error('"peer.setCapabilities" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Create a Transport instance.
	 *
	 * @param {[Object]} [options]
	 * @param {[Boolean]} [options.udp=true] - Listen UDP.
	 * @param {[Boolean]} [options.tcp=true] - Listen TCP.
	 * @param {[Boolean]} [options.preferIPv4=false] - Prefer IPv4 candidates.
	 * @param {[Boolean]} [options.preferIPv6=false] - Prefer IPv6 candidates.
	 * @param {[Boolean]} [options.preferUdp=false] - Prefer UDP candidates.
	 * @param {[Boolean]} [options.preferTcp=false] - Prefer TCP candidates.
	 *
	 * @return {Promise} Resolves to the created Transport.
	 */
	createTransport(options)
	{
		logger.debug('createTransport() [options:%o]', options);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		const internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			transportId : utils.randomNumber()
		};

		return this._channel.request('peer.createTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"peer.createTransport" request succeeded');

				// Create a Transport instance.
				const transport = new Transport(internal, data, this._channel);

				// Store the Transport instance and remove it when closed.
				this._transports.add(transport);
				transport.on('close', () => this._transports.delete(transport));

				this.emit('newtransport', transport);

				return transport;
			})
			.catch((error) =>
			{
				logger.error('"peer.createTransport" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Create a RtpReceiver instance.
	 *
	 * @param {Transport} transport - Transport instance.
	 *
	 * @return {RtpReceiver}
	 */
	RtpReceiver(kind, transport)
	{
		logger.debug('RtpReceiver() [kind:%s]', kind);

		if (this._closed)
			throw new errors.InvalidStateError('Peer closed');

		// Ensure `kind` is 'audio' / 'video' / 'depth'.
		if (KINDS.indexOf(kind) === -1)
			throw new TypeError(`unsupported kind: ${kind}`);

		// Ensure `transport` is given.
		if (!transport)
			throw new TypeError('transport not given');

		// Ensure `transport` is a Transport.
		if (!(transport instanceof Transport))
			throw new TypeError('transport must be a instance of Transport');

		// Ensure `transport` is not closed.
		if (transport.closed)
			throw new errors.InvalidStateError('transport is closed');

		// Ensure `transport` belongs to this Peer.
		if (!this._transports.has(transport))
			throw new Error('transport not found');

		const internal =
		{
			roomId        : this._internal.roomId,
			peerId        : this._internal.peerId,
			rtpReceiverId : utils.randomNumber(),
			transportId   : transport._internal.transportId
		};

		const data =
		{
			kind      : kind,
			peer      : this,
			transport : transport
		};

		// Create a RtpReceiver instance.
		const rtpReceiver = new RtpReceiver(internal, data, this._channel);

		// Store the RtpReceiver instance and remove it when closed.
		this._rtpReceivers.add(rtpReceiver);
		rtpReceiver.on('close', () => this._rtpReceivers.delete(rtpReceiver));

		const options =
		{
			kind : kind
		};

		this._channel.request('peer.createRtpReceiver', internal, options)
			.then(() =>
			{
				logger.debug('"peer.createRtpReceiver" request succeeded');

				this.emit('newrtpreceiver', rtpReceiver);
			})
			.catch((error) =>
			{
				logger.error('"peer.createRtpReceiver" request failed: %s', error);

				rtpReceiver.close(error, true);
			});

		return rtpReceiver;
	}
}

module.exports = Peer;
