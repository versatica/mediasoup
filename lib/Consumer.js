'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');

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
		// - .source
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Locally paused flag.
		// @type {Boolean}
		this._locallyPaused = false;

		// Remotely paused flag.
		// @type {Boolean}
		this._remotelyPaused = false;

		// Source paused flag.
		// @type {Boolean}
		this._sourcePaused = false;

		this._handleNotifications();
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

	get source()
	{
		return this._data.source;
	}

	/**
	 * Whether the Consumer is locally paused.
	 *
	 * @return {Boolean}
	 */
	get locallyPaused()
	{
		return this._locallyPaused;
	}

	/**
	 * Whether the Consumer is remotely paused.
	 *
	 * @return {Boolean}
	 */
	get remotelyPaused()
	{
		return this._remotelyPaused;
	}

	/**
	 * Whether the source (Producer) is paused.
	 *
	 * @return {Boolean}
	 */
	get sourcePaused()
	{
		return this._sourcePaused;
	}

	/**
	 * Whether the Consumer is paused.
	 *
	 * @return {Boolean}
	 */
	get paused()
	{
		return this._locallyPaused || this._remotelyPaused || this._sourcePaused;
	}

	/**
	 * Close the Consumer.
	 *
	 * @private
	 */
	close()
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		this.emit('@notify', 'consumerClosed', { id: this.id });

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
	 * Start sending media to the remote Consumer.
	 *
	 * @param {Transport} transport - Transport instance.
	 * @param {RTCRtpParameters} rtpParameters - Sending RTP parameters.
	 *
	 * @return {Promise} Resolves to this.
	 */
	enable(transport, rtpParameters)
	{
		logger.debug('enable()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Consumer closed'));
		else if (this.transport)
			return Promise.reject(new errors.InvalidStateError('Consumer already enabled'));

		this._internal.transportId = transport.id;

		const data =
		{
			rtpParameters
		};

		return this._channel.request('consumer.enable', this._internal, data)
			.then(() =>
			{
				logger.debug('"consumer.enable" request succeeded');

				this._data.transport = transport;
				this._data.rtpParameters = rtpParameters;

				// On Transport closure reset the Consumer.
				transport.on('@close', () =>
				{
					this._internal.transportId = undefined;
				});

				return this;
			})
			.catch((error) =>
			{
				logger.error('"consumer.enable" request failed: %s', String(error));

				this._internal.transportId = undefined;

				throw error;
			});
	}

	/**
	 * Pauses sending media.
	 *
	 * @param {Any} [appData] - App custom data.
	 *
	 * @return {Boolean} true if paused.
	 */
	pause(appData)
	{
		logger.debug('pause()');

		if (this._closed)
		{
			logger.error('pause() | Consumer closed');

			return false;
		}
		else if (this._locallyPaused)
		{
			return true;
		}

		if (!this._sourcePaused)
			this.emit('@notify', 'consumerPaused', { id: this.id, appData });

		this._locallyPaused = true;

		// Send Channel request.
		this._channel.request('consumer.pause', this._internal)
			.then(() =>
			{
				logger.debug('"consumer.pause" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"consumer.pause" request failed: %s', String(error));
			});

		this.safeEmit('pause', 'local', appData);

		// Return true if really paused.
		return this.paused;
	}

	/**
	 * The remote Consumer was paused.
	 * Invoked via remote notification.
	 *
	 * @private
	 *
	 * @param {Any} [appData] - App custom data.
	 */
	remotePause(appData)
	{
		logger.debug('remotePause()');

		if (this._closed || this._remotelyPaused)
			return;

		this._remotelyPaused = true;

		// Send Channel request.
		this._channel.request('consumer.pause', this._internal)
			.then(() =>
			{
				logger.debug('"consumer.pause" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"consumer.pause" request failed: %s', String(error));
			});

		this.safeEmit('pause', 'remote', appData);
	}

	/**
	 * Resumes sending media.
	 *
	 * @param {Any} [appData] - App custom data.
	 *
	 * @return {Boolean} true if not paused.
	 */
	resume(appData)
	{
		logger.debug('resume()');

		if (this._closed)
		{
			logger.error('resume() | Consumer closed');

			return false;
		}
		else if (!this._locallyPaused)
		{
			return true;
		}

		if (!this._sourcePaused)
			this.emit('@notify', 'consumerResumed', { id: this.id, appData });

		this._locallyPaused = false;

		if (!this._remotelyPaused)
		{
			// Send Channel request.
			this._channel.request('consumer.resume', this._internal)
				.then(() =>
				{
					logger.debug('"consumer.resume" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"consumer.resume" request failed: %s', String(error));
				});
		}

		this.safeEmit('resume', 'local', appData);

		// Return true if not paused.
		return !this.paused;
	}

	/**
	 * The remote Consumer was resumed.
	 * Invoked via remote notification.
	 *
	 * @private
	 *
	 * @param {Any} [appData] - App custom data.
	 */
	remoteResume(appData)
	{
		logger.debug('remoteResume()');

		if (this._closed || !this._remotelyPaused)
			return;

		this._remotelyPaused = false;

		if (!this._locallyPaused)
		{
			// Send Channel request.
			this._channel.request('consumer.resume', this._internal)
				.then(() =>
				{
					logger.debug('"consumer.resume" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"consumer.resume" request failed: %s', String(error));
				});
		}

		this.safeEmit('resume', 'remote', appData);
	}

	_handleNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.consumerId, (event) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close();

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}
}

module.exports = Consumer;
