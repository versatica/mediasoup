const Logger = require('./Logger');
const Transport = require('./Transport');

const logger = new Logger('WebRtcTransport');

class WebRtcTransport extends Transport
{
	/**
	 * @private
	 *
	 * @emits {iceState: String} icestatechange
	 * @emits {iceSelectedTuple: Object} iceselectedtuplechange
	 * @emits {dtlsState: String} dtlsstatechange
	 * @emits {iceState: String} observer:icestatechange
	 * @emits {iceSelectedTuple: Object} observer:iceselectedtuplechange
	 * @emits {dtlsState: String} observer:dtlsstatechange
	 */
	constructor({ data, ...params })
	{
		super(params);

		logger.debug('constructor()');

		// WebRtcTransport data.
		// @type {Object}
		// - .iceRole
		// - .iceParameters
		//   - .usernameFragment
		//   - .password
		//   - .iceLite
		// - .iceCandidates []
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
		// - .dtlsParameters
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
			iceRole          : data.iceRole,
			iceParameters    : data.iceParameters,
			iceCandidates    : data.iceCandidates,
			iceState         : data.iceState,
			iceSelectedTuple : data.iceSelectedTuple,
			dtlsParameters   : data.dtlsParameters,
			dtlsState        : data.dtlsState,
			dtlsRemoteCert   : data.dtlsRemoteCert
		};
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
	get iceParameters()
	{
		return this._data.iceParameters;
	}

	/**
	 * @returns {Array<RTCIceCandidate>}
	 */
	get iceCandidates()
	{
		return this._data.iceCandidates;
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
	get dtlsParameters()
	{
		return this._data.dtlsParameters;
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

		this._data.iceState = 'closed';
		this._data.iceSelectedTuple = undefined;
		this._data.dtlsState = 'closed';

		super.close();
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

		this._data.iceState = 'closed';
		this._data.iceSelectedTuple = undefined;
		this._data.dtlsState = 'closed';

		super.routerClosed();
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

		// Update data.
		this._data.dtlsParameters.role = data.dtlsLocalRole;
	}

	/**
	 * Set maximum incoming bitrate for receiving media.
	 *
	 * @param {Number} bitrate - In bps.
	 *
	 * @async
	 */
	async setMaxIncomingBitrate(bitrate)
	{
		logger.debug('setMaxIncomingBitrate() [bitrate:%s]', bitrate);

		const reqData = { bitrate };

		return this._channel.request(
			'transport.setMaxIncomingBitrate', this._internal, reqData);
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

		const { iceParameters } = data;

		this._data.iceParameters = iceParameters;

		return iceParameters;
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
				case 'icestatechange':
				{
					const { iceState } = data;

					this._data.iceState = iceState;

					this.safeEmit('icestatechange', iceState);

					// Emit observer event.
					this.safeEmit('observer:icestatechange', iceState);

					break;
				}

				case 'iceselectedtuplechange':
				{
					const { iceSelectedTuple } = data;

					this._data.iceSelectedTuple = iceSelectedTuple;

					this.safeEmit('iceselectedtuplechange', iceSelectedTuple);

					// Emit observer event.
					this.safeEmit('observer:iceselectedtuplechange', iceSelectedTuple);

					break;
				}

				case 'dtlsstatechange':
				{
					const { dtlsState, dtlsRemoteCert } = data;

					this._data.dtlsState = dtlsState;

					if (dtlsState === 'connected')
						this._data.dtlsRemoteCert = dtlsRemoteCert;

					this.safeEmit('dtlsstatechange', dtlsState);

					// Emit observer event.
					this.safeEmit('observer:dtlsstatechange', dtlsState);

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
