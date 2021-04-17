import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import {
	Transport,
	TransportListenIp,
	TransportProtocol,
	TransportTuple,
	TransportTraceEventData,
	SctpState
} from './Transport';
import { SctpParameters, NumSctpStreams } from './SctpParameters';

export type WebRtcTransportOptions =
{
	/**
	 * Listening IP address or addresses in order of preference (first one is the
	 * preferred one).
	 */
	listenIps: (TransportListenIp | string)[];

	/**
	 * Listen in UDP. Default true.
	 */
	enableUdp?: boolean;

	/**
	 * Listen in TCP. Default false.
	 */
	enableTcp?: boolean;

	/**
	 * Prefer UDP. Default false.
	 */
	preferUdp?: boolean;

	/**
	 * Prefer TCP. Default false.
	 */
	preferTcp?: boolean;

	/**
	 * Initial available outgoing bitrate (in bps). Default 600000.
	 */
	initialAvailableOutgoingBitrate?: number;

	/**
	 * Create a SCTP association. Default false.
	 */
	enableSctp?: boolean;

	/**
	 * SCTP streams number.
	 */
	numSctpStreams?: NumSctpStreams;

	/**
	 * Maximum allowed size for SCTP messages sent by DataProducers.
	 * Default 262144.
	 */
	maxSctpMessageSize?: number;

	/**
	 * Maximum SCTP send buffer used by DataConsumers.
	 * Default 262144.
	 */
	sctpSendBufferSize?: number;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

export type IceParameters =
{
	usernameFragment: string;
	password: string;
	iceLite?: boolean;
}

export type IceCandidate =
{
	foundation: string;
	priority: number;
	ip: string;
	protocol: TransportProtocol;
	port: number;
	type: 'host';
	tcpType: 'passive' | undefined;
}

export type DtlsParameters =
{
	role?: DtlsRole;
	fingerprints: DtlsFingerprint[];
}

/**
 * The hash function algorithm (as defined in the "Hash function Textual Names"
 * registry initially specified in RFC 4572 Section 8) and its corresponding
 * certificate fingerprint value (in lowercase hex string as expressed utilizing
 * the syntax of "fingerprint" in RFC 4572 Section 5).
 */
export type DtlsFingerprint =
{
	algorithm: string;
	value: string;
}

export type IceState = 'new' | 'connected' | 'completed' | 'disconnected' | 'closed';

export type DtlsRole = 'auto' | 'client' | 'server';

export type DtlsState = 'new' | 'connecting' | 'connected' | 'failed' | 'closed';

export type WebRtcTransportStat =
{
	// Common to all Transports.
	type: string;
	transportId: string;
	timestamp: number;
	sctpState?: SctpState;
	bytesReceived: number;
	recvBitrate: number;
	bytesSent: number;
	sendBitrate: number;
	rtpBytesReceived: number;
	rtpRecvBitrate: number;
	rtpBytesSent: number;
	rtpSendBitrate: number;
	rtxBytesReceived: number;
	rtxRecvBitrate: number;
	rtxBytesSent: number;
	rtxSendBitrate: number;
	probationBytesSent: number;
	probationSendBitrate: number;
	availableOutgoingBitrate?: number;
	availableIncomingBitrate?: number;
	maxIncomingBitrate?: number;
	// WebRtcTransport specific.
	iceRole: string;
	iceState: IceState;
	iceSelectedTuple?: TransportTuple;
	dtlsState: DtlsState;
}

const logger = new Logger('WebRtcTransport');

export class WebRtcTransport extends Transport
{
	// WebRtcTransport data.
	protected readonly _data:
	{
		iceRole: 'controlled';
		iceParameters: IceParameters;
		iceCandidates: IceCandidate[];
		iceState: IceState;
		iceSelectedTuple?: TransportTuple;
		dtlsParameters: DtlsParameters;
		dtlsState: DtlsState;
		dtlsRemoteCert?: string;
		sctpParameters?: SctpParameters;
		sctpState?: SctpState;
	};

	/**
	 * @private
	 * @emits icestatechange - (iceState: IceState)
	 * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
	 * @emits dtlsstatechange - (dtlsState: DtlsState)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	constructor(params: any)
	{
		super(params);

		logger.debug('constructor()');

		const { data } = params;

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

		this._handleWorkerNotifications();
	}

	/**
	 * ICE role.
	 */
	get iceRole(): 'controlled'
	{
		return this._data.iceRole;
	}

	/**
	 * ICE parameters.
	 */
	get iceParameters(): IceParameters
	{
		return this._data.iceParameters;
	}

	/**
	 * ICE candidates.
	 */
	get iceCandidates(): IceCandidate[]
	{
		return this._data.iceCandidates;
	}

	/**
	 * ICE state.
	 */
	get iceState(): IceState
	{
		return this._data.iceState;
	}

	/**
	 * ICE selected tuple.
	 */
	get iceSelectedTuple(): TransportTuple | undefined
	{
		return this._data.iceSelectedTuple;
	}

	/**
	 * DTLS parameters.
	 */
	get dtlsParameters(): DtlsParameters
	{
		return this._data.dtlsParameters;
	}

	/**
	 * DTLS state.
	 */
	get dtlsState(): DtlsState
	{
		return this._data.dtlsState;
	}

	/**
	 * Remote certificate in PEM format.
	 */
	get dtlsRemoteCert(): string | undefined
	{
		return this._data.dtlsRemoteCert;
	}

	/**
	 * SCTP parameters.
	 */
	get sctpParameters(): SctpParameters | undefined
	{
		return this._data.sctpParameters;
	}

	/**
	 * SCTP state.
	 */
	get sctpState(): SctpState | undefined
	{
		return this._data.sctpState;
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @emits close
	 * @emits newproducer - (producer: Producer)
	 * @emits newconsumer - (consumer: Consumer)
	 * @emits newdataproducer - (dataProducer: DataProducer)
	 * @emits newdataconsumer - (dataConsumer: DataConsumer)
	 * @emits icestatechange - (iceState: IceState)
	 * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
	 * @emits dtlsstatechange - (dtlsState: DtlsState)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	get observer(): EnhancedEventEmitter
	{
		return this._observer;
	}

	/**
	 * Close the WebRtcTransport.
	 *
	 * @override
	 */
	close(): void
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
	routerClosed(): void
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
	 * Get WebRtcTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<WebRtcTransportStat[]>
	{
		logger.debug('getStats()');

		return this._channel.request('transport.getStats', this._internal);
	}

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @override
	 */
	async connect({ dtlsParameters }: { dtlsParameters: DtlsParameters }): Promise<void>
	{
		logger.debug('connect()');

		const reqData = { dtlsParameters };

		const data =
			await this._channel.request('transport.connect', this._internal, reqData);

		// Update data.
		this._data.dtlsParameters.role = data.dtlsLocalRole;
	}

	/**
	 * Restart ICE.
	 */
	async restartIce(): Promise<IceParameters>
	{
		logger.debug('restartIce()');

		const data =
			await this._channel.request('transport.restartIce', this._internal);

		const { iceParameters } = data;

		this._data.iceParameters = iceParameters;

		return iceParameters;
	}

	private _handleWorkerNotifications(): void
	{
		this._channel.on(this._internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case 'icestatechange':
				{
					const iceState = data.iceState as IceState;

					this._data.iceState = iceState;

					this.safeEmit('icestatechange', iceState);

					// Emit observer event.
					this._observer.safeEmit('icestatechange', iceState);

					break;
				}

				case 'iceselectedtuplechange':
				{
					const iceSelectedTuple = data.iceSelectedTuple as TransportTuple;

					this._data.iceSelectedTuple = iceSelectedTuple;

					this.safeEmit('iceselectedtuplechange', iceSelectedTuple);

					// Emit observer event.
					this._observer.safeEmit('iceselectedtuplechange', iceSelectedTuple);

					break;
				}

				case 'dtlsstatechange':
				{
					const dtlsState = data.dtlsState as DtlsState;
					const dtlsRemoteCert = data.dtlsRemoteCert as string;

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
					const sctpState = data.sctpState as SctpState;

					this._data.sctpState = sctpState;

					this.safeEmit('sctpstatechange', sctpState);

					// Emit observer event.
					this._observer.safeEmit('sctpstatechange', sctpState);

					break;
				}

				case 'trace':
				{
					const trace = data as TransportTraceEventData;

					this.safeEmit('trace', trace);

					// Emit observer event.
					this._observer.safeEmit('trace', trace);

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
