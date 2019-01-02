const Logger = require('./Logger');
const Transport = require('./Transport');

const logger = new Logger('WebRtcTransport');

class WebRtcTransport extends Transport
{
	constructor({ internal, data, channel, appData })
	{
		super({ internal, channel, appData });

		logger.debug('constructor()');

		// WebRtcTransport data.
		// @type {Object}
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
		//   - .localIp
		//   - .localPort
		//   - .remoteIp
		//   - .remotePort
		//   - .protocol
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
	 * @override
	 */
	close()
	{
		if (this._closed)
			return;

		super.close();

		delete this._data.iceSelectedTuple;
		this._data.iceState = 'closed';
		this._data.dtlsState = 'closed';
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @override
	 */
	routerClosed()
	{
		if (this._closed)
			return;

		super.routerClosed();

		delete this._data.iceSelectedTuple;
		this._data.iceState = 'closed';
		this._data.dtlsState = 'closed';
	}

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @param {Object} transportRemoteParameters - Remote parameters.
	 *
	 * @async
	 * @override
	 */
	async connect({ transportRemoteParameters })
	{
		logger.debug('connect()');

		// TODO: NO.
		const data = { transportRemoteParameters };

		const transportData =
			await this._channel.request('transport.connect', this._internal, data);

		// Take the role field from the response.
		this._data.dtlsLocalParameters.role = transportData.role;
	}

	/**
	 * Set maximum bitrate (in bps).
	 *
	 * @param {Number} bitrate
	 *
	 * @async
	 */
	async setMaxBitrate(bitrate)
	{
		logger.debug('setMaxBitrate() [bitrate:%s]', bitrate);

		const data = { bitrate };

		return this._channel.request(
			'transport.setMaxBitrate', this._internal, data);
	}

	/**
	 * TODO
	 *
	 * Tell the WebRtcTransport to generate new uFrag and password values.
	 *
	 * @private
	 *
	 * @return {Promise} Resolves to this.
	 */
	async changeUfragPwd()
	{
		logger.debug('changeUfragPwd()');

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
	 * @private
	 * @override
	 */
	_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.transportId, (event, data) =>
		{
			switch (event)
			{
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
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});
	}
}

module.exports = WebRtcTransport;
