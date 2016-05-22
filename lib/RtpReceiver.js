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

		// Store internal data.
		// - .roomId
		// - .peerId
		// - .rtpReceiverId
		// - .transportId
		this._internal = internal;

		// RtcReceiver data.
		// - .kind
		// - .transport
		// - .rtpParameters
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Closed flag.
		this._closed = false;

		// Subscribe to notifications.
		this._channel.on(this._internal.rtpReceiverId, (event, data, buffer) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, true);
					break;
				}

				case 'rtpraw':
				{
					this.emit('rtpraw', buffer);
					break;
				}

				case 'rtpobject':
				{
					// Append the binary buffer into `object.payload`.
					data.object.payload = buffer;
					this.emit('rtpobject', data.object);
					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});

		// Subscribe to new events so we can handle 'rtpraw' and 'rtpobject'.
		this.on('newListener', (event) =>
		{
			switch (event)
			{
				case 'rtpraw':
				{
					// Ignore if there are listeners already.
					if (this.listenerCount('rtpraw'))
						return;

					// Send Channel request.
					this._channel.request('rtpReceiver.setRtpRawEvent', this._internal, { enabled: true })
						.then(() =>
						{
							logger.debug('"rtpReceiver.setRtpRawEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error('"rtpReceiver.setRtpRawEvent" request failed: %s', error);
						});

					break;
				}

				case 'rtpobject':
				{
					// Ignore if there are listeners already.
					if (this.listenerCount('rtpobject'))
						return;

					// Send Channel request.
					this._channel.request('rtpReceiver.setRtpObjectEvent', this._internal, { enabled: true })
						.then(() =>
						{
							logger.debug('"rtpReceiver.setRtpObjectEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error('"rtpReceiver.setRtpObjectEvent" request failed: %s', error);
						});

					break;
				}
			}
		});

		// Subscribe to events removal so we can handle 'rtpraw' and 'rtpobject'.
		this.on('removeListener', (event) =>
		{
			switch (event)
			{
				case 'rtpraw':
				{
					// Ignore if there are other remaining listeners.
					if (this.listenerCount('rtpraw'))
						return;

					// Send Channel request.
					this._channel.request('rtpReceiver.setRtpRawEvent', this._internal, { enabled: false })
						.then(() =>
						{
							logger.debug('"rtpReceiver.setRtpRawEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error('"rtpReceiver.setRtpRawEvent" request failed: %s', error);
						});

					break;
				}

				case 'rtpobject':
				{
					// Ignore if there are other remaining listeners.
					if (this.listenerCount('rtpobject'))
						return;

					// Send Channel request.
					this._channel.request('rtpReceiver.setRtpObjectEvent', this._internal, { enabled: false })
						.then(() =>
						{
							logger.debug('"rtpReceiver.setRtpObjectEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error('"rtpReceiver.setRtpObjectEvent" request failed: %s', error);
						});

					break;
				}
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

	get transport()
	{
		return this._data.transport;
	}

	get rtpParameters()
	{
		return this._data.rtpParameters;
	}

	/**
	 * Close the RtpReceiver.
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
		this._channel.removeAllListeners(this._internal.rtpReceiverId);

		if (!dontSendChannel)
		{
			// Send Channel request.
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
	 * Dump the RtpReceiver.
	 *
	 * @return {Promise}
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
	 * Start receiving media.
	 *
	 * @param {Object} rtpParameters
	 *
	 * @return {Promise} Resolves to this.
	 */
	receive(rtpParameters)
	{
		logger.debug('receive() [rtpParameters:%o]', rtpParameters);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpReceiver closed'));

		// Send Channel request.
		return this._channel.request('rtpReceiver.receive', this._internal, rtpParameters)
			.then((data) =>
			{
				logger.debug('"rtpReceiver.receive" request succeeded');

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
