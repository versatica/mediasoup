'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('RtpReceiver');
const errors = require('./errors');

class RtpReceiver extends EventEmitter
{
	constructor(internal, data, channel)
	{
		logger.debug('constructor() [internal:%o, data:%o]', internal, data);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data
		// - .roomId
		// - .peerId
		// - .rtpReceiverId
		// - .transportId
		this._internal = internal;

		// RtcReceiver data
		// - .rtpParameters
		// - .transport
		this._data = data;

		// Channel instance
		this._channel = channel;

		// Flag indicating RTP receipt
		this._listenForRtp = false;

		// Closed flag
		this._closed = false;

		// Subscribe to notifications
		this._channel.on(this._internal.rtpReceiverId, (event, data) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, true);
					break;
				}

				case 'rtp':
				{
					this.emit('rtp', data);
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

	get listenForRtp()
	{
		return this._listenForRtp;
	}

	set listenForRtp(enabled)
	{
		enabled = !!enabled;

		logger.debug('set listenForRtp [enabled:%s]', enabled);

		if (this._closed)
			return;

		if (enabled === this._listenForRtp)
			return;

		let data =
		{
			enabled : enabled
		};

		// Send Channel request
		this._channel.request('rtpReceiver.listenForRtp', this._internal, data)
			.then(() =>
			{
				logger.debug('"rtpReceiver.listenForRtp" request succeeded');

				this._listenForRtp = enabled;
			})
			.catch((error) =>
			{
				logger.error('"rtpReceiver.listenForRtp" request failed: %s', error);
			});
	}

	/**
	 * Close the RtpReceiver
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
		this._channel.removeAllListeners(this._internal.rtpReceiverId);

		if (!dontSendChannel)
		{
			// Send Channel request
			this._channel.request('rtpReceiver.close', this._internal)
				.then(() =>
				{
					logger.debug('"rtpReceiver.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"rtpReceiver.close" request failed: %s', error);
				});
		}

		this.emit('close', error);
	}

	/**
	 * Dump the RtpReceiver
	 * @return {Promise}  Resolves to object data
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpReceiver closed'));

		return this._channel.request('rtpReceiver.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"rtpReceiver.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"rtpReceiver.dump" request failed: %s', error);

				throw error;
			});
	}

	isRtpReceiver()
	{
		return true;
	}

	isRtpSender()
	{
		return false;
	}

	/**
	 * Start receiving media
	 * @param  {Object} rtpParameters
	 * @return {Promise}  Resolves to this
	 */
	receive(rtpParameters)
	{
		logger.debug('receive() [rtpParameters:%o]', rtpParameters);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpReceiver closed'));

		// Send Channel request
		return this._channel.request('rtpReceiver.receive', this._internal, rtpParameters)
			.then((data) =>
			{
				logger.debug('"rtpReceiver.receive" request succeeded');

				// Set rtpParameters
				this._data.rtpParameters = data;

				return this;
			})
			.catch((error) =>
			{
				logger.error('"rtpReceiver.receive" request failed: %s', error);

				throw error;
			});
	}
}

module.exports = RtpReceiver;
