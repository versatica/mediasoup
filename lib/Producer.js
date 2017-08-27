'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');

const logger = new Logger('Producer');

class Producer extends EnhancedEventEmitter
{
	constructor(internal, data, channel)
	{
		super(logger);

		logger.debug('constructor()');

		// Internal data.
		// - .roomId
		// - .peerId
		// - .producerId
		// - .transportId
		this._internal = internal;

		// Producer data.
		// - .kind
		// - .transport
		// - .rtpParameters
		// - .adaptedRtpParameters
		// - .rtpMapping
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Closed flag.
		this._closed = false;

		// Subscribe to notifications.
		this._channel.on(this._internal.producerId, (event, data2, buffer) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, false);

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
					data2.object.payload = buffer;

					this.emit('rtpobject', data2.object);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});

		// Subscribe to new events.
		this.on('newListener', (event) =>
		{
			switch (event)
			{
				case 'rtpraw':
				{
					// Send Channel request.
					this._channel.request(
						'producer.setRtpRawEvent', this._internal, { enabled: true })
						.then(() =>
						{
							logger.debug('"producer.setRtpRawEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error(
								'"producer.setRtpRawEvent" request failed: %s', String(error));
						});

					break;
				}

				case 'rtpobject':
				{
					// Send Channel request.
					this._channel.request(
						'producer.setRtpObjectEvent', this._internal, { enabled: true })
						.then(() =>
						{
							logger.debug('"producer.setRtpObjectEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error(
								'"producer.setRtpObjectEvent" request failed: %s', String(error));
						});

					break;
				}
			}
		});

		// Subscribe to events removal.
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
					this._channel.request(
						'producer.setRtpRawEvent', this._internal, { enabled: false })
						.then(() =>
						{
							logger.debug('"producer.setRtpRawEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error(
								'"producer.setRtpRawEvent" request failed: %s', String(error));
						});

					break;
				}

				case 'rtpobject':
				{
					// Ignore if there are other remaining listeners.
					if (this.listenerCount('rtpobject'))
						return;

					// Send Channel request.
					this._channel.request(
						'producer.setRtpObjectEvent', this._internal, { enabled: false })
						.then(() =>
						{
							logger.debug('"producer.setRtpObjectEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error(
								'"producer.setRtpObjectEvent" request failed: %s', String(error));
						});

					break;
				}
			}
		});
	}

	get id()
	{
		return this._internal.producerId;
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

	get adaptedRtpParameters()
	{
		return this._data.adaptedRtpParameters;
	}

	get rtpMapping()
	{
		return this._data.rtpMapping;
	}

	/**
	 * Close the Producer.
	 *
	 * @param {Any} [appData] - App custom data.
	 * @param {Boolean} [notifyChannel=true] - Private.
	 */
	close(appData, notifyChannel = true)
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		this.emit(
			'@notify', 'producerClosed', { id: this.id, appData });

		this.emit('@close');
		this.safeEmit('close', 'local', appData);

		this._destroy(notifyChannel);
	}

	/**
	 * The remote Producer was closed.
	 * Invoked via remote notification.
	 *
	 * @private
	 *
	 * @param {Any} [appData] - App custom data.
	 * @param {Boolean} [notifyChannel=true] - Private.
	 */
	remoteClose(appData, notifyChannel = true)
	{
		logger.debug('remoteClose()');

		if (this.closed)
			return;

		this._closed = true;

		this.emit('@close');
		this.safeEmit('close', 'remote', appData);

		this._destroy(notifyChannel);
	}

	_destroy(notifyChannel = true)
	{
		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.producerId);

		if (notifyChannel)
		{
			// Send Channel request.
			this._channel.request('producer.close', this._internal)
				.then(() =>
				{
					logger.debug('"producer.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"producer.close" request failed: %s', String(error));
				});
		}
	}

	/**
	 * Dump the Producer.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Producer closed'));

		return this._channel.request('producer.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"producer.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"producer.dump" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Start receiving media from the remote Producer.
	 *
	 * @param {RTCRtpParameters} rtpParameters - Receiving RTP parameters.
	 * @param {RTCRtpParameters} adaptedRtpParameters - RTP parameters adapted to the
	 * room RTP capabilities.
	 * @param {Object} rtpMapping.
	 *
	 * @return {Promise} Resolves to this.
	 */
	receive(rtpParameters, adaptedRtpParameters, rtpMapping)
	{
		logger.debug('receive()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Producer closed'));

		const data =
		{
			rtpParameters,
			rtpMapping
		};

		// Send Channel request.
		return this._channel.request('producer.receive', this._internal, data)
			.then(() =>
			{
				logger.debug('"producer.receive" request succeeded');

				this._data.rtpParameters = rtpParameters;
				this._data.adaptedRtpParameters = adaptedRtpParameters;
				this._data.rtpMapping = rtpMapping;

				return this;
			})
			.catch((error) =>
			{
				logger.error('"producer.receive" request failed: %s', String(error));

				throw error;
			});
	}
}

module.exports = Producer;
