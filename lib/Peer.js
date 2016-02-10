'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Peer');
const utils = require('./utils');
const errors = require('./errors');
const Transport = require('./Transport');
const RtpReceiver = require('./RtpReceiver');
const RtpSender = require('./RtpSender');

class Peer extends EventEmitter
{
	constructor(internal, channel)
	{
		logger.debug('constructor() [internal:%o]', internal);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data
		// - .roomId
		// - .peerId
		this._internal = internal;

		// Channel instance
		this._channel = channel;

		// Set of Transport instances
		this._transports = new Set();

		// Set of RtpReceiver instances
		this._rtpReceivers = new Set();

		// Set of RtpSender instances
		this._rtpSenders = new Set();

		// Closed flag
		this._closed = false;

		// Subscribe to notifications
		this._channel.on(this._internal.peerId, (event) =>
		{
			switch (event)
			{
				case 'close':
					this.close(undefined, true);
					break;

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
	 * Close the Peer
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
		this._channel.removeAllListeners(this._internal.peerId);

		// Close every Transport
		this._transports.forEach((transport) => transport.close(undefined, true));

		// Close every RtpReceiver
		this._rtpReceivers.forEach((rtpReceiver) => rtpReceiver.close(undefined, true));

		// Close every RtpSender
		this._rtpSenders.forEach((rtpSender) => rtpSender.close());

		if (!dontSendChannel)
		{
			let internal = this._internal;

			// Send Channel request
			this._channel.request('peer.close', internal)
				.then(() =>
				{
					logger.debug('"peer.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"peer.close" request failed: %s', error);
				});
		}

		this.emit('close', error);
	}

	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		let internal = this._internal;

		return this._channel.request('peer.dump', internal)
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
	 * Create a Transport instance
	 * @param  {Object} options  Transport options
	 * @param  {Boolean} options.udp  Listen UDP (default true)
	 * @param  {Boolean} options.tcp  Listen TCP (default true)
	 * @param  {Boolean} options.preferIPv4  Prefer IPv4 candidates (default false)
	 * @param  {Boolean} options.preferIPv6  Prefer IPv6 candidates (default false)
	 * @param  {Boolean} options.preferUdp  Prefer UDP candidates (default false)
	 * @param  {Boolean} options.preferTcp  Prefer TCP candidates (default false)
	 * @return {Promise}
	 */
	createTransport(options)
	{
		logger.debug('createTransport() [options:%o]', options);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		let internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			transportId : utils.randomNumber()
		};

		return this._channel.request('peer.createTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"peer.createTransport" request succeeded');

				// Create a Transport instance
				let transport = new Transport(internal, data, this._channel);

				// Store the Transport instance and remove it when closed
				this._transports.add(transport);
				transport.on('close', () => this._transports.delete(transport));

				return transport;
			})
			.catch((error) =>
			{
				logger.error('"peer.createTransport" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Create a RtpReceiver instance
	 * @param  {Transport} transport  Transport instance
	 * @return {RtpReceiver}
	 */
	RtpReceiver(transport)
	{
		logger.debug('RtpReceiver()');

		if (this._closed)
			throw new errors.InvalidStateError('Peer closed');

		// Ensure `transport` is a Transport
		if (!(transport instanceof Transport))
			throw new TypeError('`transport` must be a instance of Transport');

		// Ensure `transport` is not closed
		if (transport.closed)
			throw new errors.InvalidStateError('`transport` is closed');

		// Ensure `transport` belongs to this Peer
		if (!this._transports.has(transport))
			throw new Error('`transport` not found');

		let internal =
		{
			roomId        : this._internal.roomId,
			peerId        : this._internal.peerId,
			rtpReceiverId : utils.randomNumber(),
			transportId   : transport._internal.transportId
		};

		let data =
		{
			transport : transport
		};

		// Create a RtpReceiver instance
		let rtpReceiver = new RtpReceiver(internal, data, this._channel);

		// Store the RtpReceiver instance and remove it when closed
		this._rtpReceivers.add(rtpReceiver);
		rtpReceiver.on('close', () => this._rtpReceivers.delete(rtpReceiver));

		this._channel.request('peer.createRtpReceiver', internal)
			.then(() =>
			{
				logger.debug('"peer.createRtpReceiver" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"peer.createRtpReceiver" request failed: %s', error);

				rtpReceiver.close(error, true);
			});

		return rtpReceiver;
	}

	/**
	 * Create a RtpSender instance
	 * This is a private method called from the Room
	 * @param  {Number} rtpSenderId
	 * @param  {Object} rtpParameters
	 * @param  {Peer} receiverPeer
	 */
	RtpSender(rtpSenderId, rtpParameters, receiverPeer)
	{
		logger.debug('RtpSender() [rtpSenderId:%s, rtpParameters:%o]', rtpSenderId, rtpParameters);

		if (this._closed)
			throw new errors.InvalidStateError('Peer closed');

		let internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			rtpSenderId : rtpSenderId,
			transportId : null
		};

		let data =
		{
			rtpParameters : rtpParameters
		};

		// Create a RtpSender instance
		let rtpSender = new RtpSender(internal, data, this._channel);

		// Store the RtpSender instance and remove it when closed
		this._rtpSenders.add(rtpSender);
		rtpSender.on('close', () => this._rtpSenders.delete(rtpSender));

		this.emit('newrtpsender', rtpSender, receiverPeer);
	}
}

module.exports = Peer;
