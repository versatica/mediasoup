'use strict';

const EventEmitter = require('events').EventEmitter;
const logger = require('./logger')('Transport');
const errors = require('./errors');

class Transport extends EventEmitter
{
	constructor(internal, data, channel)
	{
		logger.debug('constructor() [internal:%o, data:%o]', internal, data);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data.
		// - .roomId
		// - .peerId
		// - .transportId
		this._internal = internal;

		// Transport data provided by the worker.
		// - .iceRole
		// - .iceLocalParameters
		//   - .usernameFragment
		//   - .password
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

		// Closed flag.
		this._closed = false;

		// Subscribe to notifications.
		this._channel.on(this._internal.transportId, (event, data2) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, true);
					break;
				}

				case 'iceselectedtuplechange':
				{
					this._data.iceSelectedTuple = data2.iceSelectedTuple;
					// Emit it to the app.
					this.emit(event, data2.iceSelectedTuple);
					break;
				}

				case 'icestatechange':
				{
					this._data.iceState = data2.iceState;
					// Emit it to the app.
					this.emit(event, data2.iceState);
					break;
				}

				case 'dtlsstatechange':
				{
					this._data.dtlsState = data2.dtlsState;
					if (data2.dtlsState === 'connected')
						this._data.dtlsRemoteCert = data2.dtlsRemoteCert;
					// Emit it to the app.
					this.emit(event, data2.dtlsState);
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
		this._channel.removeAllListeners(this._internal.transportId);

		// We won't receive events anymore, so be nice and manually update some
		// fields.
		delete this._data.iceSelectedTuple;
		this._data.iceState = 'closed';
		this._data.dtlsState = 'closed';

		if (!dontSendChannel)
		{
			// Send Channel request.
			this._channel.request('transport.close', this._internal)
				.then(() =>
				{
					logger.debug('"transport.close" request succeeded');
				})
				.catch((error2) =>
				{
					logger.error('"transport.close" request failed: %s', error2);
				});
		}

		this.emit('close', error);
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
				logger.error('"transport.dump" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Provide the remote DTLS parameters.
	 *
	 * @param {Object} options - Remote DTLS parameters.
	 *
	 * @return {Promise} Resolves to this.
	 */
	setRemoteDtlsParameters(options)
	{
		logger.debug('setRemoteDtlsParameters() [options:%o]', options);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Transport closed'));

		// Send Channel request.
		return this._channel.request('transport.setRemoteDtlsParameters', this._internal, options)
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
				logger.error('"transport.setRemoteDtlsParameters" request failed: %s', error);

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
				logger.error('"transport.setMaxBitrate" request failed: %s', error);

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
					password         : data.password
				};

				return this;
			})
			.catch((error) =>
			{
				logger.error('"transport.changeUfragPwd" request failed: %s', error);

				throw error;
			});
	}
}

module.exports = Transport;
