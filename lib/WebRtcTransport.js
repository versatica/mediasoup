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
	 * @emits {sctpState: String} sctpstatechange
	 */
	constructor(params)
	{
		super(params);

		logger.debug('constructor()');

		const { data } = params;

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
		// - .sctpParameters
		//   - .port
		//   - .OS
		//   - .MIS
		//   - .maxMessageSize
		// - .sctpState
		this._data =
		{
			iceRole          : data.iceRole,
			iceParameters    : data.iceParameters,
			iceCandidates    : data.iceCandidates,
			iceState         : data.iceState,
			iceSelectedTuple : data.iceSelectedTuple,
			dtlsParameters   : data.dtlsParameters,
			dtlsState        : data.dtlsState,
			dtlsRemoteCert   : data.dtlsRemoteCert,
			sctpParameters   : data.sctpParameters,
			sctpState        : data.sctpState
		};
	}

	/**
	 * @type {String}
	 */
	get iceRole()
	{
		return this._data.iceRole;
	}

	/**
	 * @type {Object}
	 */
	get iceParameters()
	{
		return this._data.iceParameters;
	}

	/**
	 * @type {Array<RTCIceCandidate>}
	 */
	get iceCandidates()
	{
		return this._data.iceCandidates;
	}

	/**
	 * @type {String}
	 */
	get iceState()
	{
		return this._data.iceState;
	}

	/**
	 * @type {Object}
	 */
	get iceSelectedTuple()
	{
		return this._data.iceSelectedTuple;
	}

	/**
	 * @type {Object}
	 */
	get dtlsParameters()
	{
		return this._data.dtlsParameters;
	}

	/**
	 * @type {String}
	 */
	get dtlsState()
	{
		return this._data.dtlsState;
	}

	/**
	 * @type {String}
	 */
	get dtlsRemoteCert()
	{
		return this._data.dtlsRemoteCert;
	}

	/**
	 * @type {Object}
	 */
	get sctpParameters()
	{
		return this._data.sctpParameters;
	}

	/**
	 * @type {String}
	 */
	get sctpState()
	{
		return this._data.sctpState;
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @type {EventEmitter}
	 *
	 * @emits close
	 * @emits {producer: Producer} newproducer
	 * @emits {consumer: Consumer} newconsumer
	 * @emits {producer: DataProducer} newdataproducer
	 * @emits {consumer: DataConsumer} newdataconsumer
	 * @emits {iceState: String} icestatechange
	 * @emits {iceSelectedTuple: Object} iceselectedtuplechange
	 * @emits {dtlsState: String} dtlsstatechange
	 * @emits {sctpState: String} sctpstatechange
	 */
	get observer()
	{
		return this._observer;
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

		if (this._data.sctpState)
			this._data.sctpState = 'closed';

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

		if (this._data.sctpState)
			this._data.sctpState = 'closed';

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

		await this._channel.request(
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
					this._observer.safeEmit('icestatechange', iceState);

					break;
				}

				case 'iceselectedtuplechange':
				{
					const { iceSelectedTuple } = data;

					this._data.iceSelectedTuple = iceSelectedTuple;

					this.safeEmit('iceselectedtuplechange', iceSelectedTuple);

					// Emit observer event.
					this._observer.safeEmit('iceselectedtuplechange', iceSelectedTuple);

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
					this._observer.safeEmit('dtlsstatechange', dtlsState);

					break;
				}

				case 'sctpstatechange':
				{
					const { sctpState } = data;

					this._data.sctpState = sctpState;

					this.safeEmit('sctpstatechange', sctpState);

					// Emit observer event.
					this._observer.safeEmit('sctpstatechange', sctpState);

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
