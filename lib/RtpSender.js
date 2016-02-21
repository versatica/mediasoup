'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('RtpSender');
const utils = require('./utils');
const errors = require('./errors');
const Transport = require('./Transport');

class RtpSender extends EventEmitter
{
	constructor(internal, data, channel)
	{
		logger.debug('constructor() [internal:%o, data:%o]', internal, data);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data
		// - .roomId
		// - .peerId
		// - .rtpSenderId
		// - .transportId
		this._internal = internal;

		// RtcReceiver data
		// - .transport
		// - .rtpParameters
		this._data = data;

		// Channel instance
		this._channel = channel;

		// Closed flag
		this._closed = false;

		// Subscribe to notifications
		this._channel.on(this._internal.rtpSenderId, (event, data) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close();
					break;
				}

				case 'updateparameters':
				{
					this._data.rtpParameters = data.rtpParameters;
					this.emit('updateparameters', data.rtpParameters);
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

	get rtpParameters()
	{
		return this._data.rtpParameters;
	}

	get transport()
	{
		return this._data.transport;
	}

	/**
	 * Close the Transport
	 * This is a private method as the app is not supposed to close this
	 */
	close()
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		// Remove notification subscriptions
		this._channel.removeAllListeners(this._internal.rtpSenderId);

		this.emit('close');
	}

	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpSender closed'));

		let internal = this._internal;

		return this._channel.request('rtpSender.dump', internal)
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

	setTransport(transport)
	{
		logger.debug('setTransport()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpSender closed'));

		// Ensure `transport` is given
		if (!transport)
			return Promise.reject(new TypeError('`transport` not given'));

		// Ensure `transport` is a Transport
		if (!(transport instanceof Transport))
			return Promise.reject(new TypeError('`transport` must be a instance of Transport'));

		// Ensure `transport` is not closed
		if (transport.closed)
			return Promise.reject(new errors.InvalidStateError('`transport` is closed'));

		let internal = utils.cloneObject(this._internal);

		internal.transportId = transport._internal.transportId;

		return this._channel.request('rtpSender.setTransport', internal)
			.then(() =>
			{
				logger.debug('"rtpSender.setTransport" request succeeded');

				// Update this._data.,transport
				this._data.transport = transport;
			})
			.catch((error) =>
			{
				logger.error('"rtpSender.setTransport" request failed: %s', error);

				throw error;
			});
	}
}

module.exports = RtpSender;
