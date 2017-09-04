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
		// - .routerId
		// - .consumerId
		// - .producerId
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

		// App data.
		this._appData = undefined;

		// Locally paused flag.
		// @type {Boolean}
		this._locallyPaused = false;

		// Remotely paused flag.
		// @type {Boolean}
		this._remotelyPaused = false;

		// Source paused flag.
		// @type {Boolean}
		this._sourcePaused = false;

		this._handleWorkerNotifications();
	}

	get id()
	{
		return this._internal.consumerId;
	}

	get closed()
	{
		return this._closed;
	}

	get appData()
	{
		return this._appData;
	}

	set appData(appData)
	{
		this._appData = appData;
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

	get enabled()
	{
		return Boolean(this._data.transport);
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
	 *
	 * @param {Boolean} [notifyChannel=true] - Private.
	 */
	close(notifyChannel = true)
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		const data =
		{
			id       : this.id,
			peerName : this.peer.name
		};

		// Don't notify `consumerClosed` if the Peer is already closed.
		if (!this.peer.closed)
			this.emit('@notify', 'consumerClosed', data);

		this.emit('@close');
		this.safeEmit('close');

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.consumerId);

		if (notifyChannel)
		{
			this._channel.request('consumer.close', this._internal)
				.then(() =>
				{
					logger.debug('"consumer.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"consumer.close" request failed: %s', String(error));
				});
		}
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

	toJson()
	{
		return {
			id            : this.id,
			kind          : this.kind,
			peerName      : this.peer.name,
			rtpParameters : this.rtpParameters,
			paused        : this.paused,
			appData       : this.appData
		};
	}

	/**
	 * Enable the Consumer for sending media.
	 *
	 * @param {Transport} transport
	 * @param {Boolean} remotelyPaused
	 */
	enable(transport, remotelyPaused)
	{
		logger.debug('enable()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Consumer closed'));
		else if (this.enabled)
			return Promise.reject(new errors.InvalidStateError('Consumer already enabled'));

		if (remotelyPaused)
			this.remotePause();

		const internal = Object.assign({}, this._internal,
			{
				transportId : transport.id
			});

		const data = Object.assign({}, this._data, { transport });

		return this._channel.request('consumer.enable', internal,
			{
				rtpParameters : this.rtpParameters
			})
			.then(() =>
			{
				logger.debug('"consumer.enable" request succeeded');

				this._internal = internal;
				this._data = data;

				transport.on('@close', () =>
				{
					this._internal.transportId = undefined;
					this._data.transport = undefined;
				});

				return;
			})
			.catch((error) =>
			{
				logger.error('"consumer.enable" request failed: %s', String(error));

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

		this._locallyPaused = true;

		if (this.enabled && !this._sourcePaused)
		{
			const data =
			{
				id       : this.id,
				peerName : this.peer.name,
				appData  : appData
			};

			this.emit('@notify', 'consumerPaused', data);
		}

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

		this._locallyPaused = false;

		if (this.enabled && !this._sourcePaused)
		{
			const data =
			{
				id       : this.id,
				peerName : this.peer.name,
				appData  : appData
			};

			this.emit('@notify', 'consumerResumed', data);
		}

		if (!this._remotelyPaused)
		{
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

	_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.consumerId, (event) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(false);

					break;
				}

				case 'sourcepaused':
				{
					if (this._sourcePaused)
						return;

					this._sourcePaused = true;

					if (this.enabled && !this._locallyPaused)
					{
						const data =
						{
							id       : this.id,
							peerName : this.peer.name
						};

						this.emit('@notify', 'consumerPaused', data);
					}

					this.safeEmit('pause', 'source');

					break;
				}

				case 'sourceresumed':
				{
					if (!this._sourcePaused)
						return;

					this._sourcePaused = false;

					if (this.enabled && !this._locallyPaused)
					{
						const data =
						{
							id       : this.id,
							peerName : this.peer.name
						};

						this.emit('@notify', 'consumerResumed', data);
					}

					this.safeEmit('resume', 'source');

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}
}

module.exports = Consumer;
