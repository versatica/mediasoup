'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('RtpReceiver');
const errors = require('./errors');

class RtpReceiver extends EventEmitter
{
	constructor(internal, channel)
	{
		logger.debug('constructor() [internal:%o]', internal);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data
		// - .roomId
		// - .peerId
		// - .transportId
		// - .rtcpTransportId
		// - .rtpReceiverId
		this._internal = internal;

		// Channel instance
		this._channel = channel;

		// Closed flag
		this._closed = false;

		// Subscribe to notifications
		this._channel.on(this._internal.rtpReceiverId, (event) =>
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
	 * Close the Transport
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
			let internal = this._internal;

			// Send Channel request
			this._channel.request('rtpReceiver.close', internal)
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

	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpReceiver closed'));

		let internal = this._internal;

		return this._channel.request('rtpReceiver.dump', internal)
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

	/**
	 * Start receiving media
	 * @param  {Object} parameters
	 * @return {Promise}
	 */
	receive(parameters)
	{
		logger.debug('receive() [parameters:%o]', parameters);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpReceiver closed'));

		let internal = this._internal;

		// Send Channel request
		return this._channel.request('rtpReceiver.receive', internal, parameters)
			.then(() =>
			{
				logger.debug('"rtpReceiver.receive" request succeeded');

				// Return the rtpReceiver itself so the user can handle it in the next
				// `then()` level`
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
