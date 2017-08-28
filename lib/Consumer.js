'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');
const Transport = require('./Transport');

const logger = new Logger('Consumer');

class Consumer extends EnhancedEventEmitter
{
	constructor(internal, data, channel)
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		this._closed = false;

		// Internal data.
		// - .roomId
		// - .peerId
		// - .consumerId
		// - .transportId
		this._internal = internal;

		// Consumer data.
		// - .kind
		// - .peer
		// - .transport
		// - .rtpParameters
		// - .associatedProducer
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Subscribe to notifications.
		this._channel.on(this._internal.consumerId, (event) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(); // TODO

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}

	get id()
	{
		return this._internal.consumerId;
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

	get associatedProducer()
	{
		return this._data.associatedProducer;
	}

	/**
	 * Close the Consumer.
	 * The app is not supposed to call this method.
	 * @private
	 */
	close()
	{
		if (this._closed)
			return;

		this._closed = true;

		logger.debug('close()');

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.consumerId);

		this.emit('close');
	}

	/**
	 * Dump the Consumer.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Consumer closed'));

		return this._channel.request('consumer.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"consumer.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"consumer.dump" request failed: %s', String(error));

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
			return Promise.reject(new errors.InvalidStateError('Consumer closed'));

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

		return this._channel.request('consumer.setTransport', this._internal)
			.then(() =>
			{
				logger.debug('"consumer.setTransport" request succeeded');

				this._data.transport = transport;

				this.emit('transport', transport);

				return this;
			})
			.catch((error) =>
			{
				logger.error('"consumer.setTransport" request failed: %s', String(error));

				this._internal.transportId = previousTransportId;

				throw error;
			});
	}

	/**
	 * Enable.
	 *
	 * @return {Promise} Resolves to this.
	 */
	enable(options)
	{
		logger.debug('enable() [options:%o]', options);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Consumer closed'));

		options = options || {};

		// Send Channel request.
		return this._channel.request('consumer.disable', this._internal,
			{
				disabled : false,
				emit     : Boolean(options.emit)
			})
			.then(() =>
			{
				logger.debug('"consumer.disable" request succeeded');

				return this;
			})
			.catch((error) =>
			{
				logger.error('"consumer.disable" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Disable.
	 *
	 * @return {Promise} Resolves to this.
	 */
	disable(options)
	{
		logger.debug('disable() [options:%o]', options);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Consumer closed'));

		options = options || {};

		// Send Channel request.
		return this._channel.request('consumer.disable', this._internal,
			{
				disabled : true,
				emit     : Boolean(options.emit)
			})
			.then(() =>
			{
				logger.debug('"consumer.disable" request succeeded');

				return this;
			})
			.catch((error) =>
			{
				logger.error('"consumer.disable" request failed: %s', String(error));

				throw error;
			});
	}
}

module.exports = Consumer;
