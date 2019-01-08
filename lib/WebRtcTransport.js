const Logger = require('./Logger');
const Transport = require('./Transport');

const logger = new Logger('WebRtcTransport');

class WebRtcTransport extends Transport
{
	constructor({ ...params, data })
	{
		super(params);

		logger.debug('constructor()');

		// WebRtcTransport data.
		// @type {Object}
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
		this._data =
		{
			iceRole             : data.iceRole,
			iceLocalParameters  : data.iceLocalParameters,
			iceLocalCandidates  : data.iceLocalCandidates,
			iceState            : data.iceState,
			iceSelectedTuple    : data.iceSelectedTuple,
			dtlsLocalParameters : data.dtlsLocalParameters,
			dtlsState           : data.dtlsState,
			dtlsRemoteCert      : data.dtlsRemoteCert
		};
	}

	/**
	 * @override
	 * @returns {String}
	 */
	get type()
	{
		return 'webrtc';
	}

	/**
	 * @returns {String}
	 */
	get iceRole()
	{
		return this._data.iceRole;
	}

	/**
	 * @returns {Object}
	 */
	get iceLocalParameters()
	{
		return this._data.iceLocalParameters;
	}

	/**
	 * @returns {Array<RTCIceCandidate>}
	 */
	get iceLocalCandidates()
	{
		return this._data.iceLocalCandidates;
	}

	/**
	 * @returns {String}
	 */
	get iceState()
	{
		return this._data.iceState;
	}

	/**
	 * @returns {Object}
	 */
	get iceSelectedTuple()
	{
		return this._data.iceSelectedTuple;
	}

	/**
	 * @returns {Object}
	 */
	get dtlsLocalParameters()
	{
		return this._data.dtlsLocalParameters;
	}

	/**
	 * @returns {String}
	 */
	get dtlsState()
	{
		return this._data.dtlsState;
	}

	/**
	 * @returns {String}
	 */
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

		this._data.iceState = 'closed';
		delete this._data.iceSelectedTuple;
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

		this._data.iceState = 'closed';
		delete this._data.iceSelectedTuple;
		this._data.dtlsState = 'closed';
	}

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @param {RTCDtlsParameters} dtlsParameters - Remote DTLS parameters.
	 *
	 * @async
	 * @override
	 */
	async connect({ dtlsParameters })
	{
		logger.debug('connect()');

		const reqData = { dtlsParameters };

		const data =
			await this._channel.request('transport.connect', this._internal, reqData);

		// Take the role field from the response.
		this._data.dtlsLocalParameters.role = data.role;
	}

	/**
	 * Set maximum bitrate for receiving media.
	 *
	 * @param {Number} bitrate in bps.
	 *
	 * @async
	 */
	async setMaxBitrate(bitrate)
	{
		logger.debug('setMaxBitrate() [bitrate:%s]', bitrate);

		const reqData = { bitrate };

		return this._channel.request(
			'transport.setMaxBitrate', this._internal, reqData);
	}

	/**
	 * Restart ICE.
	 *
	 * @async
	 * @returns {RTCIceParameters}
	 */
	async restartIce()
	{
		logger.debug('restartIce()');

		const data =
			await this._channel.request('transport.restartIce', this._internal);

		this._data.iceLocalParameters = data;

		return data;
	}

	/**
	 * @private
	 * @override
	 */
	_handleWorkerNotifications()
	{
		this._channel.on(this._internal.transportId, (event, data) =>
		{
			switch (event)
			{
				case 'iceselectedtuplechange':
				{
					this._data.iceSelectedTuple = data.iceSelectedTuple;

					this.safeEmit('iceselectedtuplechange', data.iceSelectedTuple);

					break;
				}

				case 'icestatechange':
				{
					this._data.iceState = data.iceState;

					this.safeEmit('icestatechange', data.iceState);

					break;
				}

				case 'dtlsstatechange':
				{
					this._data.dtlsState = data.dtlsState;

					if (data.dtlsState === 'connected')
						this._data.dtlsRemoteCert = data.dtlsRemoteCert;

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
