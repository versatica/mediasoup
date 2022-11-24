import * as flatbuffers from 'flatbuffers';
import { Logger } from './Logger';
import {
	parseBaseTransportDump,
	BaseTransportDump,
	Transport,
	TransportListenIp,
	TransportProtocol,
	TransportTuple,
	TransportTraceEventData,
	TransportEvents,
	TransportObserverEvents,
	TransportConstructorOptions,
	SctpState
} from './Transport';
import { WebRtcServer } from './WebRtcServer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import * as FbsRequest from './fbs/request_generated';
import * as FbsTransport from './fbs/transport_generated';
import { Either, parseVector } from './utils';

export type WebRtcTransportListenIndividual =
{
	/**
	 * Listening IP address or addresses in order of preference (first one is the
	 * preferred one). Mandatory unless webRtcServer is given.
	 */
	listenIps: (TransportListenIp | string)[];

	/**
	 * Fixed port to listen on instead of selecting automatically from Worker's port
	 * range.
	 */
	port?: number;
};

export type WebRtcTransportListenServer =
{
	/**
	 * Instance of WebRtcServer. Mandatory unless listenIps is given.
	 */
	webRtcServer: WebRtcServer;
};

export type WebRtcTransportListen =
	Either<WebRtcTransportListenIndividual, WebRtcTransportListenServer>;

export type WebRtcTransportOptionsBase =
{
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
	appData?: Record<string, unknown>;
};

export type WebRtcTransportOptions = WebRtcTransportOptionsBase & WebRtcTransportListen;

export type IceParameters =
{
	usernameFragment: string;
	password: string;
	iceLite?: boolean;
};

export type IceCandidate =
{
	foundation: string;
	priority: number;
	ip: string;
	protocol: TransportProtocol;
	port: number;
	type: 'host';
	tcpType: 'passive' | undefined;
};

export type DtlsParameters =
{
	role?: DtlsRole;
	fingerprints: DtlsFingerprint[];
};

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
};

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
};

export type WebRtcTransportEvents = TransportEvents &
{
	icestatechange: [IceState];
	iceselectedtuplechange: [TransportTuple];
	dtlsstatechange: [DtlsState];
	sctpstatechange: [SctpState];
};

export type WebRtcTransportObserverEvents = TransportObserverEvents &
{
	icestatechange: [IceState];
	iceselectedtuplechange: [TransportTuple];
	dtlsstatechange: [DtlsState];
	sctpstatechange: [SctpState];
};

type WebRtcTransportConstructorOptions = TransportConstructorOptions &
{
	data: WebRtcTransportData;
};

export type WebRtcTransportData =
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

const logger = new Logger('WebRtcTransport');

export class WebRtcTransport extends
	Transport<WebRtcTransportEvents, WebRtcTransportObserverEvents>
{
	// WebRtcTransport data.
	readonly #data: WebRtcTransportData;

	/**
	 * @private
	 */
	constructor(options: WebRtcTransportConstructorOptions)
	{
		super(options);

		logger.debug('constructor()');

		const { data } = options;

		this.#data =
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

		this.handleWorkerNotifications();
	}

	/**
	 * ICE role.
	 */
	get iceRole(): 'controlled'
	{
		return this.#data.iceRole;
	}

	/**
	 * ICE parameters.
	 */
	get iceParameters(): IceParameters
	{
		return this.#data.iceParameters;
	}

	/**
	 * ICE candidates.
	 */
	get iceCandidates(): IceCandidate[]
	{
		return this.#data.iceCandidates;
	}

	/**
	 * ICE state.
	 */
	get iceState(): IceState
	{
		return this.#data.iceState;
	}

	/**
	 * ICE selected tuple.
	 */
	get iceSelectedTuple(): TransportTuple | undefined
	{
		return this.#data.iceSelectedTuple;
	}

	/**
	 * DTLS parameters.
	 */
	get dtlsParameters(): DtlsParameters
	{
		return this.#data.dtlsParameters;
	}

	/**
	 * DTLS state.
	 */
	get dtlsState(): DtlsState
	{
		return this.#data.dtlsState;
	}

	/**
	 * Remote certificate in PEM format.
	 */
	get dtlsRemoteCert(): string | undefined
	{
		return this.#data.dtlsRemoteCert;
	}

	/**
	 * SCTP parameters.
	 */
	get sctpParameters(): SctpParameters | undefined
	{
		return this.#data.sctpParameters;
	}

	/**
	 * SCTP state.
	 */
	get sctpState(): SctpState | undefined
	{
		return this.#data.sctpState;
	}

	/**
	 * Close the WebRtcTransport.
	 *
	 * @override
	 */
	close(): void
	{
		if (this.closed)
			return;

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState)
			this.#data.sctpState = 'closed';

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
		if (this.closed)
			return;

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState)
			this.#data.sctpState = 'closed';

		super.routerClosed();
	}

	/**
	 * Called when closing the associated WebRtcServer.
	 *
	 * @private
	 */
	webRtcServerClosed(): void
	{
		if (this.closed)
			return;

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState)
			this.#data.sctpState = 'closed';

		super.listenServerClosed();
	}

	/**
	 * Dump Transport.
	 */
	async dump(): Promise<any>
	{
		logger.debug('dump()');

		const response = await this.channel.requestBinary(
			FbsRequest.Method.TRANSPORT_DUMP,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode the response. */
		const dump = new FbsTransport.DumpResponse();

		response.body(dump);

		const transportDump = new FbsTransport.WebRtcTransportDump();

		dump.data(transportDump);

		return parseWebRtcTransportDump(transportDump);
	}

	/**
	 * Get WebRtcTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<WebRtcTransportStat[]>
	{
		logger.debug('getStats()');

		const response = await this.channel.requestBinary(
			FbsRequest.Method.TRANSPORT_GET_STATS,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode the response. */
		const data = new FbsTransport.GetStatsResponse();

		response.body(data);

		return JSON.parse(data.stats()!);
	}

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @override
	 */
	async connect({ dtlsParameters }: { dtlsParameters: DtlsParameters }): Promise<void>
	{
		logger.debug('connect()');

		const builder = this.channel.bufferBuilder;
		// Serialize DtlsParameters.
		const dtlsParametersOffset = serializeDtlsParameters(builder, dtlsParameters);
		// Create WebRtcTransportConnectData.
		const connectData =
			FbsTransport.ConnectWebRtcTransportData.createConnectWebRtcTransportData(
				builder, dtlsParametersOffset
			);
		// Create request.
		const requestOffset = FbsTransport.ConnectRequest.createConnectRequest(
			builder,
			FbsTransport.ConnectData.ConnectWebRtcTransportData,
			connectData);
		// Wait for response.
		const response = await this.channel.requestBinary(
			FbsRequest.Method.TRANSPORT_CONNECT,
			FbsRequest.Body.FBS_Transport_ConnectRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode the response. */
		const data = new FbsTransport.ConnectResponse();

		response.body(data);

		const webRtcTransportConnectResponse =
			new FbsTransport.ConnectWebRtcTransportResponse();

		data.data(webRtcTransportConnectResponse);

		// Update data.
		this.#data.dtlsParameters.role =
			webRtcTransportConnectResponse.dtlsLocalRole()! as DtlsRole;
	}

	/**
	 * Restart ICE.
	 */
	async restartIce(): Promise<IceParameters>
	{
		logger.debug('restartIce()');

		const response = await this.channel.requestBinary(
			FbsRequest.Method.TRANSPORT_RESTART_ICE,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode the response. */
		const restartIceResponse = new FbsTransport.RestartIceResponse();

		response.body(restartIceResponse);

		const iceParameters = {
			usernameFragment : restartIceResponse.usernameFragment()!,
			password         : restartIceResponse.password()!,
			iceLite          : restartIceResponse.iceLite()
		};

		this.#data.iceParameters = iceParameters;

		return iceParameters;
	}

	private handleWorkerNotifications(): void
	{
		this.channel.on(this.internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case 'icestatechange':
				{
					const iceState = data.iceState as IceState;

					this.#data.iceState = iceState;

					this.safeEmit('icestatechange', iceState);

					// Emit observer event.
					this.observer.safeEmit('icestatechange', iceState);

					break;
				}

				case 'iceselectedtuplechange':
				{
					const iceSelectedTuple = data.iceSelectedTuple as TransportTuple;

					this.#data.iceSelectedTuple = iceSelectedTuple;

					this.safeEmit('iceselectedtuplechange', iceSelectedTuple);

					// Emit observer event.
					this.observer.safeEmit('iceselectedtuplechange', iceSelectedTuple);

					break;
				}

				case 'dtlsstatechange':
				{
					const dtlsState = data.dtlsState as DtlsState;
					const dtlsRemoteCert = data.dtlsRemoteCert as string;

					this.#data.dtlsState = dtlsState;

					if (dtlsState === 'connected')
						this.#data.dtlsRemoteCert = dtlsRemoteCert;

					this.safeEmit('dtlsstatechange', dtlsState);

					// Emit observer event.
					this.observer.safeEmit('dtlsstatechange', dtlsState);

					break;
				}

				case 'sctpstatechange':
				{
					const sctpState = data.sctpState as SctpState;

					this.#data.sctpState = sctpState;

					this.safeEmit('sctpstatechange', sctpState);

					// Emit observer event.
					this.observer.safeEmit('sctpstatechange', sctpState);

					break;
				}

				case 'trace':
				{
					const trace = data as TransportTraceEventData;

					this.safeEmit('trace', trace);

					// Emit observer event.
					this.observer.safeEmit('trace', trace);

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

export function parseIceCandidate(binary: FbsTransport.IceCandidate): IceCandidate
{
	return {
		foundation : binary.foundation()!,
		priority   : binary.priority(),
		ip         : binary.ip()!,
		protocol   : binary.protocol() as TransportProtocol,
		port       : binary.port(),
		type       : 'host',
		tcpType    : binary.tcpType() === 'passive' ? 'passive' : undefined
	};
}

export function parseIceParameters(binary: FbsTransport.IceParameters): IceParameters
{
	return {
		usernameFragment : binary.usernameFragment()!,
		password         : binary.password()!,
		iceLite          : binary.iceLite()
	};
}

export function parseDtlsParameters(binary: FbsTransport.DtlsParameters): DtlsParameters
{
	const fingerprints: DtlsFingerprint[] = [];

	for (let i=0; i<binary.fingerprintsLength(); ++i)
	{
		const fbsFingerprint = binary.fingerprints(i)!;
		const fingerPrint : DtlsFingerprint = {
			algorithm : fbsFingerprint.algorithm()!,
			value     : fbsFingerprint.value()!
		};

		fingerprints.push(fingerPrint);
	}

	return {
		fingerprints : fingerprints,
		role         : binary.role()! as DtlsRole
	};
}

export function serializeDtlsParameters(
	builder: flatbuffers.Builder, dtlsParameters: DtlsParameters
): number
{
	const fingerprints: number[] = [];

	try
	{
		for (const fingerprint of dtlsParameters.fingerprints)
		{
			const algorithmOffset = builder.createString(fingerprint.algorithm);
			const valueOffset = builder.createString(fingerprint.value);
			const fingerprintOffset = FbsTransport.Fingerprint.createFingerprint(
				builder, algorithmOffset, valueOffset);

			fingerprints.push(fingerprintOffset);
		}

		const fingerprintsOffset = FbsTransport.DtlsParameters.createFingerprintsVector(
			builder, fingerprints);
		const roleOffset = builder.createString(dtlsParameters.role);

		return FbsTransport.DtlsParameters.createDtlsParameters(
			builder,
			fingerprintsOffset,
			roleOffset);
	}
	catch (error)
	{
		throw new TypeError(`${error}`);
	}
}

type WebRtcTransportDump = BaseTransportDump &
{
	iceRole: 'controlled';
	iceParameters: IceParameters;
	iceCandidates: IceCandidate[];
	iceState: IceState;
	iceSelectedTuple?: TransportTuple;
	dtlsParameters: DtlsParameters;
	dtlsState: DtlsState;
	dtlsRemoteCert?: string;
};

export function parseWebRtcTransportDump(
	binary: FbsTransport.WebRtcTransportDump
): WebRtcTransportDump
{
	// Retrieve BaseTransportDump.
	const fbsBaseTransportDump = new FbsTransport.BaseTransportDump();

	binary.base()!.data(fbsBaseTransportDump);
	const baseTransportDump = parseBaseTransportDump(fbsBaseTransportDump);

	// Retrieve ICE candidates.
	const iceCandidates = parseVector<IceCandidate>(binary, 'iceCandidates', parseIceCandidate);
	// Retrieve ICE parameters.
	const iceParameters = parseIceParameters(binary.iceParameters()!);
	// Retrieve DTLS parameters.
	const dtlsParameters = parseDtlsParameters(binary.dtlsParameters()!);

	return {
		...baseTransportDump,
		sctpParameters : baseTransportDump.sctpParameters,
		sctpState      : baseTransportDump.sctpState,
		iceRole        : 'controlled',
		iceParameters  : iceParameters,
		iceCandidates  : iceCandidates,
		iceState       : binary.iceState() as IceState,
		dtlsParameters : dtlsParameters,
		dtlsState      : binary.dtlsState() as DtlsState
	};
}
