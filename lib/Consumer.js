'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');

const PROFILES = new Set([ 'default', 'low', 'medium', 'high' ]);

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
		// - .peer (just if this Consumer belongs to a Peer)
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

		// Periodic stats flag.
		// @type {Boolean}
		this._statsEnabled = false;

		// Periodic stats interval identifier.
		// @type {Boolean}
		this._statsInterval = null;

		// Preferred profile.
		// @type {String}
		this._preferredProfile = 'default';

		// Effective profile.
		// @type {String}
		this._effectiveProfile = null;

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
	 * The preferred profile.
	 *
	 * @type {String}
	 */
	get preferredProfile()
	{
		return this._preferredProfile;
	}

	/**
	 * The effective profile.
	 *
	 * @type {String}
	 */
	get effectiveProfile()
	{
		return this._effectiveProfile;
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

		if (this._statsEnabled)
		{
			this._statsEnabled = false;
			clearInterval(this._statsInterval);
		}

		// Notify if it has a Peer.
		if (this.hasPeer())
		{
			const data =
			{
				id       : this.id,
				peerName : this.peer.name
			};

			// Don't notify 'consumerClosed' if the Peer is already closed.
			if (!this.peer.closed)
				this.emit('@notify', 'consumerClosed', data);
		}

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
	 * @private
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
	 * Get the Consumer stats.
	 *
	 * @return {Promise}
	 */
	getStats()
	{
		logger.debug('getStats()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Consumer closed'));

		return this._channel.request('consumer.getStats', this._internal)
			.then((data) =>
			{
				logger.debug('"consumer.getStats" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"consumer.getStats" request failed: %s', String(error));

				throw error;
			});
	}

	toJson()
	{
		const json =
		{
			id               : this.id,
			kind             : this.kind,
			rtpParameters    : this.rtpParameters,
			paused           : this.paused,
			preferredProfile : this.preferredProfile,
			effectiveProfile : this.effectiveProfile,
			appData          : this.appData
		};

		if (this.hasPeer())
			json.peerName = this.peer.name;

		return json;
	}

	/**
	 * Whether this Consumer belongs to a Peer.
	 *
	 * @return {Boolean}
	 */
	hasPeer()
	{
		return Boolean(this.peer);
	}

	/**
	 * Enable the Consumer for sending media.
	 *
	 * @param {Transport} transport
	 * @param {Boolean} remotelyPaused
	 * @param {String} preferredProfile
	 */
	enable(transport, remotelyPaused, preferredProfile)
	{
		logger.debug('enable()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Consumer closed'));
		else if (this.enabled)
			return Promise.reject(new errors.InvalidStateError('Consumer already enabled'));

		if (remotelyPaused)
			this.remotePause();

		if (preferredProfile)
			this.remoteSetPreferredProfile(preferredProfile);

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

		// Notify if it has a Peer.
		if (this.hasPeer())
		{
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
	 * Enables periodic stats retrieval.
	 *
	 */
	enableStats(appData)
	{
		logger.debug('enableStats()');

		const { interval } = appData;

		if (this._closed)
		{
			logger.error('enableStats() | Consumer closed');

			return;
		}

		if (this._statsEnabled)
			return;

		this._statsEnabled = true;

		this._statsInterval = setInterval(() =>
		{
			if (this.paused || !this.enabled)
				return;

			this.getStats()
				.then((stats) =>
				{
					const data =
					{
						id       : this.id,
						peerName : this.peer.name,
						stats
					};

					this.emit('@notify', 'consumerStats', data);
				})
				.catch((error) =>
				{
					logger.error('"getStats" request failed: %s', String(error));
				});
		}, interval * 1000);
	}

	/**
	 * Disables periodic stats retrieval.
	 *
	 */
	disableStats()
	{
		logger.debug('disableStats()');

		if (this._closed)
		{
			logger.error('disableStats() | Consumer closed');

			return;
		}

		if (!this._statsEnabled)
			return;

		this._statsEnabled = false;
		clearInterval(this._statsInterval);
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

		// Notify if it has a Peer.
		if (this.hasPeer())
		{
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

	/**
	 * Sets the preferred RTP profile.
	 *
	 * @param {String} profile
	 */
	setPreferredProfile(profile)
	{
		logger.debug('setPreferredProfile() [profile:%s]', profile);

		if (this._closed)
		{
			logger.error('setPreferredProfile() | Consumer closed');

			return;
		}
		else if (profile === this._preferredProfile)
		{
			return;
		}
		else if (!PROFILES.has(profile))
		{
			logger.error('setPreferredProfile() | invalid profile "%s"', profile);

			return;
		}

		this._channel.request('consumer.setPreferredProfile', this._internal,
			{
				profile
			})
			.then(() =>
			{
				logger.debug('"consumer.setPreferredProfile" request succeeded');

				this._preferredProfile = profile;

				// Notify if it has a Peer.
				if (this.hasPeer() && !this._closed)
				{
					const data =
					{
						id       : this.id,
						peerName : this.peer.name,
						profile  : profile
					};

					this.emit('@notify', 'consumerPreferredProfileSet', data);
				}
			})
			.catch((error) =>
			{
				logger.error(
					'"consumer.setPreferredProfile" request failed: %s', String(error));
			});
	}

	/**
	 * Preferred receiving profile was set on my remote Consumer.
	 *
	 * @param {String} profile
	 */
	remoteSetPreferredProfile(profile)
	{
		logger.debug('remoteSetPreferredProfile() [profile:%s]', profile);

		if (this._closed || profile === this._preferredProfile)
			return;

		this._channel.request('consumer.setPreferredProfile', this._internal,
			{
				profile
			})
			.then(() =>
			{
				logger.debug('"consumer.setPreferredProfile" request succeeded');

				this._preferredProfile = profile;
			})
			.catch((error) =>
			{
				logger.error(
					'"consumer.setPreferredProfile" request failed: %s', String(error));
			});
	}

	/**
	 * Sets the encoding preferences.
	 * Only for testing.
	 *
	 * @private
	 *
	 * @param {String} profile
	 */
	setEncodingPreferences(preferences)
	{
		logger.debug('setEncodingPreferences() [preferences:%o]', preferences);

		if (this._closed)
		{
			logger.error('setEncodingPreferences() | Consumer closed');

			return;
		}

		this._channel.request(
			'consumer.setEncodingPreferences', this._internal, preferences)
			.then(() =>
			{
				logger.debug('"consumer.setEncodingPreferences" request succeeded');
			})
			.catch((error) =>
			{
				logger.error(
					'"consumer.setEncodingPreferences" request failed: %s', String(error));
			});
	}

	/**
	 * Request a key frame to the source.
	 */
	requestKeyFrame()
	{
		logger.debug('requestKeyFrame()');

		if (this._closed || !this.enabled || this.paused)
			return;

		if (this.kind !== 'video')
			return;

		this._channel.request('consumer.requestKeyFrame', this._internal)
			.then(() =>
			{
				logger.debug('"consumer.requestKeyFrame" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"consumer.requestKeyFrame" request failed: %s', String(error));
			});
	}

	_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.consumerId, (event, data) =>
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

					// Notify if it has a Peer.
					if (this.hasPeer())
					{
						if (this.enabled && !this._locallyPaused)
						{
							const data2 =
							{
								id       : this.id,
								peerName : this.peer.name
							};

							this.emit('@notify', 'consumerPaused', data2);
						}
					}

					this.safeEmit('pause', 'source');

					break;
				}

				case 'sourceresumed':
				{
					if (!this._sourcePaused)
						return;

					this._sourcePaused = false;

					// Notify if it has a Peer.
					if (this.hasPeer())
					{
						if (this.enabled && !this._locallyPaused)
						{
							const data2 =
							{
								id       : this.id,
								peerName : this.peer.name
							};

							this.emit('@notify', 'consumerResumed', data2);
						}
					}

					this.safeEmit('resume', 'source');

					break;
				}

				case 'effectiveprofilechange':
				{
					const { profile } = data;

					if (this._effectiveProfile === profile)
						return;

					this._effectiveProfile = profile;

					// Notify if it has a Peer.
					if (this.hasPeer())
					{
						if (this.enabled)
						{
							const data2 =
							{
								id       : this.id,
								peerName : this.peer.name,
								profile  : this._effectiveProfile
							};

							this.emit('@notify', 'consumerEffectiveProfileChanged', data2);
						}
					}

					this.safeEmit('effectiveprofilechange', this._effectiveProfile);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}
}

module.exports = Consumer;
