'use strict';

const EventEmitter = require('events').EventEmitter;
const logger = require('./logger')('RtpSender');
const errors = require('./errors');
const Transport = require('./Transport');

class RtpSender extends EventEmitter
{
	constructor(internal, data, channel)
	{
		logger.debug('constructor() [internal:%o, data:%o]', internal, data);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data.
		// - .roomId
		// - .peerId
		// - .rtpSenderId
		// - .transportId
		this._internal = internal;

		// RtcSender data.
		// - .kind
		// - .associatedPeer
		// - .transport
		// - .rtpParameters
		// - .avaiable
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Closed flag
		this._closed = false;

		// Subscribe to notifications.
		this._channel.on(this._internal.rtpSenderId, (event, data) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close();
					break;
				}

				case 'parameterschange':
				{
					this._data.rtpParameters = data.rtpParameters;
					this._data.available = data.available;
					this.emit('parameterschange', data.rtpParameters, data.available);
					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}

	get closed()
	{
		return this._closed;
	}

	get kind()
	{
		return this._data.kind;
	}

	get associatedPeer()
	{
		return this._data.associatedPeer;
	}

	get transport()
	{
		return this._data.transport;
	}

	get rtpParameters()
	{
		return this._data.rtpParameters;
	}

	get available()
	{
		return this._data.available;
	}

	/**
	 * Close the RtpSender.
	 * The app is not supposed to call this method.
	 * @private
	 */
	close()
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.rtpSenderId);

		this.emit('close');
	}

	/**
	 * Dump the RtpSender.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpSender closed'));

		return this._channel.request('rtpSender.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"rtpSender.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"rtpSender.dump" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Assign Transport.
	 *
	 * @param {Transport} transport - Transport instance.
	 *
	 * @return {Promise} Resolves to this.
	 */
	setTransport(transport)
	{
		logger.debug('setTransport()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpSender closed'));

		// Ensure `transport` is given.
		if (!transport)
			return Promise.reject(new TypeError('transport not given'));

		// Ensure `transport` is a Transport.
		if (!(transport instanceof Transport))
			return Promise.reject(new TypeError('transport must be a instance of Transport'));

		// Ensure `transport` is not closed.
		if (transport.closed)
			return Promise.reject(new errors.InvalidStateError('transport is closed'));

		this._internal.transportId = transport._internal.transportId;

		return this._channel.request('rtpSender.setTransport', this._internal)
			.then(() =>
			{
				logger.debug('"rtpSender.setTransport" request succeeded');

				this._data.transport = transport;

				this.emit('transport', transport);

				return this;
			})
			.catch((error) =>
			{
				logger.error('"rtpSender.setTransport" request failed: %s', error);

				delete this._internal.transportId;

				throw error;
			});
	}
}

module.exports = RtpSender;
