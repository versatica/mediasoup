'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');

const logger = new Logger('PlainRtpTransport');

class PlainRtpTransport extends EnhancedEventEmitter
{
	constructor(internal, data, channel)
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		this._closed = false;

		// Internal data.
		// - .routerId
		// - .transportId
		this._internal = internal;

		// PlainRtpTransport data provided by the worker.
		// - .tuple
		//   - .local
		//     - .ip
		//     - .port
		//     - .protocol
		//   - .remote
		//     - .ip
		//     - .port
		//     - .protocol
		this._data = data;

		// Channel instance.
		this._channel = channel;

		this._handleWorkerNotifications();
	}

	get id()
	{
		return this._internal.transportId;
	}

	get closed()
	{
		return this._closed;
	}

	get tuple()
	{
		return this._data.tuple;
	}

	/**
	 * Close the PlainRtpTransport.
	 *
	 * @param {Boolean} [notifyChannel=true] - Private.
	 */
	close(notifyChannel = true)
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		this.emit('@close');

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.transportId);

		if (notifyChannel)
		{
			this._channel.request('transport.close', this._internal)
				.then(() =>
				{
					logger.debug('"transport.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"transport.close" request failed: %s', String(error));
				});
		}
	}

	/**
	 * Dump the PlainRtpTransport.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('PlainRtpTransport closed'));

		return this._channel.request('transport.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"transport.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"transport.dump" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Enables periodic stats retrieval.
	 *
	 * Not implemented.
	 *
	 */
	enableStats()
	{
	}

	/**
	 * Disables periodic stats retrieval.
	 *
	 * Not implemented.
	 *
	 */
	disableStats()
	{
	}

	_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.transportId, (event) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(false);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}
}

module.exports = PlainRtpTransport;
