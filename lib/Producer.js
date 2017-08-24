'use strict';

const EventEmitter = require('events').EventEmitter;
const Logger = require('./Logger');
const errors = require('./errors');
const Transport = require('./Transport');

const logger = new Logger('Producer');

class Producer extends EventEmitter
{
	constructor(internal, data, channel)
	{
		logger.debug('constructor()');

		super();
		this.setMaxListeners(Infinity);

		// Store internal data.
		// - .roomId
		// - .peerId
		// - .producerId
		// - .transportId
		this._internal = internal;

		// Producer data.
		// - .kind
		// - .peer
		// - .transport
		// - .rtpParameters
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
							logger.error('"producer.setRtpRawEvent" request failed: %s', error);
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
								'"producer.setRtpObjectEvent" request failed: %s', error);
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
							logger.error('"producer.setRtpRawEvent" request failed: %s', error);
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
								'"producer.setRtpObjectEvent" request failed: %s', error);
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

	get peer()
	{
		return this._data.peer;
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
	 * Close the Producer.
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
		this._channel.removeAllListeners(this._internal.producerId);

		if (!dontSendChannel)
		{
			// Send Channel request.
			this._channel.request('producer.close', this._internal)
				.then(() =>
				{
					logger.debug('"producer.close" request succeeded');
				})
				.catch((error2) =>
				{
					logger.error('"producer.close" request failed: %s', error2);
				});
		}

		this.emit('close', error);
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
				logger.error('"producer.dump" request failed: %s', error);

				throw error;
			});
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
			return Promise.reject(new errors.InvalidStateError('Producer closed'));

		// Send Channel request.
		return this._channel.request(
			'producer.receive', this._internal, rtpParameters)
			.then((data) =>
			{
				logger.debug('"producer.receive" request succeeded');

				const hadParameters = Boolean(this._data.rtpParameters);

				this._data.rtpParameters = data;

				if (!hadParameters)
					this.emit('parameters', data);
				else
					this.emit('parameterschange', data);

				return this;
			})
			.catch((error) =>
			{
				logger.error('"producer.receive" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Assign new Transport.
	 *
	 * @param {Transport} transport - Transport instance.
	 *
	 * @return {Promise} Resolves to this.
	 */
	setTransport(transport)
	{
		logger.debug('setTransport()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Producer closed'));

		// Ensure `transport` is given.
		if (!transport)
			return Promise.reject(new TypeError('transport not given'));

		// Ensure `transport` is a Transport.
		if (!(transport instanceof Transport))
		{
			return Promise.reject(
				new TypeError('transport must be a instance of Transport'));
		}

		// Ensure `transport` is not closed.
		if (transport.closed)
			return Promise.reject(new errors.InvalidStateError('transport is closed'));

		const previousTransportId = this._internal.transportId;

		this._internal.transportId = transport._internal.transportId;

		return this._channel.request('producer.setTransport', this._internal)
			.then(() =>
			{
				logger.debug('"producer.setTransport" request succeeded');

				this._data.transport = transport;

				this.emit('transport', transport);

				return this;
			})
			.catch((error) =>
			{
				logger.error('"producer.setTransport" request failed: %s', error);

				this._internal.transportId = previousTransportId;

				throw error;
			});
	}
}

module.exports = Producer;
