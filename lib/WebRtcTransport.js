'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');

const DEFAULT_STATS_INTERVAL = 1000;

const logger = new Logger('WebRtcTransport');

class WebRtcTransport extends EnhancedEventEmitter
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

		// WebRtcTransport data provided by the worker.
		// - .direction
		// - .iceRole
		// - .iceLocalParameters
		//   - .usernameFragment
		//   - .password
		//   - .iceLite
		// - .iceLocalCandidates []
		//   - .foundation
		//   - .priority
		//   - .ip
		//   - .port
		//   - .type
		//   - .protocol
		//   - .tcpType
		// - .iceState
		// - .iceSelectedTuple
		//   - .local
		//     - .ip
		//     - .port
		//     - .protocol
		//   - .remote
		//     - .ip
		//     - .port
		//     - .protocol
		// - .dtlsLocalParameters
		//   - .role
		//   - .fingerprints
		//     - .sha-1
		//     - .sha-224
		//     - .sha-256
		//     - .sha-384
		//     - .sha-512
		// - .dtlsState
		// - .dtlsRemoteCert
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// App data.
		this._appData = undefined;

		// Periodic stats flag.
		// @type {Boolean}
		this._statsEnabled = false;

		// Periodic stats interval identifier.
		// @type {Boolean}
		this._statsInterval = null;

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

	get appData()
	{
		return this._appData;
	}

	set appData(appData)
	{
		this._appData = appData;
	}

	get direction()
	{
		return this._data.direction;
	}

	get iceRole()
	{
		return this._data.iceRole;
	}

	get iceLocalParameters()
	{
		return this._data.iceLocalParameters;
	}

	get iceLocalCandidates()
	{
		return this._data.iceLocalCandidates;
	}

	get iceState()
	{
		return this._data.iceState;
	}

	get iceSelectedTuple()
	{
		return this._data.iceSelectedTuple;
	}

	get dtlsLocalParameters()
	{
		return this._data.dtlsLocalParameters;
	}

	get dtlsState()
	{
		return this._data.dtlsState;
	}

	get dtlsRemoteCert()
	{
		return this._data.dtlsRemoteCert;
	}

	/**
	 * Close the WebRtcTransport.
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

		if (this._statsEnabled)
		{
			this._statsEnabled = false;
			clearInterval(this._statsInterval);
		}

		this.emit('@notify', 'transportClosed', { id: this.id, appData });

		this.emit('@close');
		this.safeEmit('close', 'local', appData);

		this._destroy(notifyChannel);
	}

	/**
	 * Remote WebRtcTransport was closed.
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

		if (this._closed)
			return;

		this._closed = true;

		this.emit('@close');
		this.safeEmit('close', 'remote', appData);

		this._destroy(notifyChannel);
	}

	_destroy(notifyChannel = true)
	{
		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.transportId);

		// We won't receive events anymore, so be nice and manually update some
		// fields.
		delete this._data.iceSelectedTuple;
		this._data.iceState = 'closed';
		this._data.dtlsState = 'closed';

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
	 * Dump the WebRtcTransport.
	 *
	 * @private
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('WebRtcTransport closed'));

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
	 * Provide the remote DTLS parameters.
	 *
	 * @private
	 *
	 * @param {Object} parameters - Remote DTLS parameters.
	 *
	 * @return {Promise} Resolves to this.
	 */
	setRemoteDtlsParameters(parameters)
	{
		logger.debug('setRemoteDtlsParameters()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('WebRtcTransport closed'));

		return this._channel.request(
			'transport.setRemoteDtlsParameters', this._internal, parameters)
			.then((data) =>
			{
				logger.debug('"transport.setRemoteDtlsParameters" request succeeded');

				// Take the .role field of the response data and append it to our
				// this._data.dtlsLocalParameters.
				this._data.dtlsLocalParameters.role = data.role;

				return this;
			})
			.catch((error) =>
			{
				logger.error(
					'"transport.setRemoteDtlsParameters" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Set maximum bitrate (in bps).
	 *
	 * @return {Promise} Resolves to this.
	 */
	setMaxBitrate(bitrate)
	{
		logger.debug('setMaxBitrate() [bitrate:%s]', bitrate);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('WebRtcTransport closed'));
		else if (this.direction !== 'send')
			return Promise.reject(new Error('invalid WebRtcTransport direction'));

		return this._channel.request('transport.setMaxBitrate', this._internal,
			{
				bitrate
			})
			.then(() =>
			{
				logger.debug('"transport.setMaxBitrate" request succeeded');

				return this;
			})
			.catch((error) =>
			{
				logger.error('"transport.setMaxBitrate" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Tell the WebRtcTransport to generate new uFrag and password values.
	 *
	 * @private
	 *
	 * @return {Promise} Resolves to this.
	 */
	changeUfragPwd()
	{
		logger.debug('changeUfragPwd()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('WebRtcTransport closed'));

		return this._channel.request('transport.changeUfragPwd', this._internal)
			.then((data) =>
			{
				logger.debug('"transport.changeUfragPwd" request succeeded');

				this._data.iceLocalParameters =
				{
					usernameFragment : data.usernameFragment,
					password         : data.password,
					iceLite          : this._data.iceLocalParameters.iceLite
				};

				return this;
			})
			.catch((error) =>
			{
				logger.error('"transport.changeUfragPwd" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Get the WebRtcTransport stats.
	 *
	 * @return {Promise}
	 */
	getStats()
	{
		logger.debug('getStats()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('WebRtcTransport closed'));

		return this._channel.request('transport.getStats', this._internal)
			.then((data) =>
			{
				logger.debug('"transport.getStats" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"transport.getStats" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Enables periodic stats retrieval.
	 *
	 * @private
	 */
	enableStats(interval = DEFAULT_STATS_INTERVAL)
	{
		logger.debug('enableStats()');

		if (this._closed)
		{
			logger.error('enableStats() | WebRtcTransport closed');

			return;
		}

		if (this._statsEnabled)
			return;

		this._statsEnabled = true;

		this.getStats()
			.then((stats) =>
			{
				this.emit('@notify', 'transportStats', { id: this.id, stats });
			})
			.catch((error) =>
			{
				logger.error('"getStats" request failed: %s', String(error));
			});

		// Set minimum interval to DEFAULT_STATS_INTERVAL.
		if (interval < DEFAULT_STATS_INTERVAL)
		{
			interval = DEFAULT_STATS_INTERVAL;
		}

		this._statsInterval = setInterval(() =>
		{
			this.getStats()
				.then((stats) =>
				{
					this.emit('@notify', 'transportStats', { id: this.id, stats });
				})
				.catch((error) =>
				{
					logger.error('"getStats" request failed: %s', String(error));
				});
		}, interval);
	}

	/**
	 * Disables periodic stats retrieval.
	 *
	 * @private
	 */
	disableStats()
	{
		logger.debug('disableStats()');

		if (this._closed)
		{
			logger.error('disableStats() | WebRtcTransport closed');

			return;
		}

		if (!this._statsEnabled)
			return;

		this._statsEnabled = false;
		clearInterval(this._statsInterval);
	}

	/**
	 * Tell the WebRtcTransport to start mirroring incoming data.
	 *
	 * @return {Promise} Resolves to this.
	 */
	startMirroring(options)
	{
		logger.debug('startMirroring()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('WebRtcTransport closed'));

		return this._channel.request('transport.startMirroring', this._internal, options)
			.then(() =>
			{
				logger.debug('"transport.startMirroring" request succeeded');

				return this;
			})
			.catch((error) =>
			{
				logger.error('"transport.startMirroring" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Tell the WebRtcTransport to stop mirroring incoming data.
	 *
	 * @return {Promise} Resolves to this.
	 */
	stopMirroring()
	{
		logger.debug('stopMirroring()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('WebRtcTransport closed'));

		return this._channel.request('transport.stopMirroring', this._internal)
			.then(() =>
			{
				logger.debug('"transport.stopMirroring" request succeeded');

				return this;
			})
			.catch((error) =>
			{
				logger.error('"transport.stopMirroring" request failed: %s', String(error));

				throw error;
			});
	}

	_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.transportId, (event, data) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, false);

					break;
				}

				case 'iceselectedtuplechange':
				{
					this._data.iceSelectedTuple = data.iceSelectedTuple;

					// Emit it to the app.
					this.safeEmit('iceselectedtuplechange', data.iceSelectedTuple);

					break;
				}

				case 'icestatechange':
				{
					this._data.iceState = data.iceState;

					// Emit it to the app.
					this.safeEmit('icestatechange', data.iceState);

					break;
				}

				case 'dtlsstatechange':
				{
					this._data.dtlsState = data.dtlsState;

					if (data.dtlsState === 'connected')
						this._data.dtlsRemoteCert = data.dtlsRemoteCert;

					// Emit it to the app.
					this.safeEmit('dtlsstatechange', data.dtlsState);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}
}

module.exports = WebRtcTransport;
