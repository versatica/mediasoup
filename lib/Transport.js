'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');

const logger = new Logger('Transport');

class Transport extends EnhancedEventEmitter
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
		// - .transportId
		this._internal = internal;

		// Transport data provided by the worker.
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

		// Subscribe to notifications.
		this._channel.on(this._internal.transportId, (event, data2) =>
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
					this._data.iceSelectedTuple = data2.iceSelectedTuple;

					// Emit it to the app.
					this.safeEmit('iceselectedtuplechange', data2.iceSelectedTuple);

					break;
				}

				case 'icestatechange':
				{
					this._data.iceState = data2.iceState;

					// Emit it to the app.
					this.safeEmit('icestatechange', data2.iceState);

					break;
				}

				case 'dtlsstatechange':
				{
					this._data.dtlsState = data2.dtlsState;

					if (data2.dtlsState === 'connected')
						this._data.dtlsRemoteCert = data2.dtlsRemoteCert;

					// Emit it to the app.
					this.safeEmit('dtlsstatechange', data2.dtlsState);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
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
	 * Close the Transport.
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
			'@notify', 'transportClosed', { id: this.id, appData });

		this.emit('@close');
		this.safeEmit('close', 'local', appData);

		this._destroy(notifyChannel);
	}

	/**
	 * Remote Transport was closed.
	 * Invoked via remote notification.
	 *
	 * @private
	 *
	 * @param {Any} [appData] - App custom data.
	 */
	remoteClose(appData)
	{
		logger.debug('remoteClose()');

		if (this._closed)
			return;

		this._closed = true;

		this.emit('@close');
		this.safeEmit('close', 'remote', appData);

		this._destroy();
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
			// Send Channel request.
			this._channel.request('transport.close', this._internal)
				.then(() =>
				{
					logger.debug('"transport.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"transport.close" request failed: %o', error);
				});
		}
	}

	/**
	 * Dump the Transport.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Transport closed'));

		return this._channel.request('transport.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"transport.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"transport.dump" request failed: %o', error);

				throw error;
			});
	}

	/**
	 * Provide the remote DTLS parameters.
	 *
	 * @param {Object} parameters - Remote DTLS parameters.
	 *
	 * @return {Promise} Resolves to this.
	 */
	setRemoteDtlsParameters(parameters)
	{
		logger.debug('setRemoteDtlsParameters()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Transport closed'));

		// Send Channel request.
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
					'"transport.setRemoteDtlsParameters" request failed: %o', error);

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
			return Promise.reject(new errors.InvalidStateError('Transport closed'));

		const options = { bitrate };

		// Send Channel request.
		return this._channel.request('transport.setMaxBitrate', this._internal, options)
			.then(() =>
			{
				logger.debug('"transport.setMaxBitrate" request succeeded');

				return this;
			})
			.catch((error) =>
			{
				logger.error('"transport.setMaxBitrate" request failed: %o', error);

				throw error;
			});
	}

	/**
	 * Tell the transport to generate new uFrag and password values.
	 *
	 * @return {Promise} Resolves to this.
	 */
	changeUfragPwd()
	{
		logger.debug('changeUfragPwd()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Transport closed'));

		// Send Channel request.
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
				logger.error('"transport.changeUfragPwd" request failed: %o', error);

				throw error;
			});
	}
}

module.exports = Transport;
