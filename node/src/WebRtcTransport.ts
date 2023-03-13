import * as flatbuffers from 'flatbuffers';
import { Logger } from './Logger';
import {
	fbsSctpState2StcpState,
	parseBaseTransportDump,
	parseBaseTransportStats,
	parseTransportTraceEventData,
	parseTuple,
	BaseTransportDump,
	BaseTransportStats,
	Transport,
	TransportListenIp,
	TransportProtocol,
	TransportTuple,
	TransportEvents,
	TransportObserverEvents,
	TransportConstructorOptions,
	SctpState
} from './Transport';
import { WebRtcServer } from './WebRtcServer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import { Event, Notification } from './fbs/notification';
import * as FbsRequest from './fbs/request';
import * as FbsTransport from './fbs/transport';
import * as FbsWebRtcTransport from './fbs/web-rtc-transport';
import { DtlsState as FbsDtlsState } from './fbs/web-rtc-transport/dtls-state';
import { IceState as FbsIceState } from './fbs/web-rtc-transport/ice-state';
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

export type WebRtcTransportStat = BaseTransportStats &
{
	type: string;
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
		{
			return;
		}

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState)
		{
			this.#data.sctpState = 'closed';
		}

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
		{
			return;
		}

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState)
		{
			this.#data.sctpState = 'closed';
		}

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
		{
			return;
		}

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState)
		{
			this.#data.sctpState = 'closed';
		}

		super.listenServerClosed();
	}

	/**
	 * Dump Transport.
	 */
	async dump(): Promise<WebRtcTransportDump>
	{
		logger.debug('dump()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_DUMP,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsWebRtcTransport.DumpResponse();

		response.body(data);

		return parseWebRtcTransportDumpResponse(data);
	}

	/**
	 * Get WebRtcTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<WebRtcTransportStat[]>
	{
		logger.debug('getStats()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_GET_STATS,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsWebRtcTransport.GetStatsResponse();

		response.body(data);

		return [ parseGetStatsResponse(data) ];
	}

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @override
	 */
	async connect({ dtlsParameters }: { dtlsParameters: DtlsParameters }): Promise<void>
	{
		logger.debug('connect()');

		const requestOffset = createConnectRequest({
			builder : this.channel.bufferBuilder,
			dtlsParameters
		});

		// Wait for response.
		const response = await this.channel.request(
			FbsRequest.Method.WEBRTC_TRANSPORT_CONNECT,
			FbsRequest.Body.FBS_WebRtcTransport_ConnectRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsWebRtcTransport.ConnectResponse();

		response.body(data);

		// Update data.
		this.#data.dtlsParameters.role =
			data.dtlsLocalRole()! as DtlsRole;
	}

	/**
	 * Restart ICE.
	 */
	async restartIce(): Promise<IceParameters>
	{
		logger.debug('restartIce()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_RESTART_ICE,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
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
		this.channel.on(this.internal.transportId, (event: Event, data?: Notification) =>
		{
			switch (event)
			{
				case Event.WEBRTCTRANSPORT_ICE_STATE_CHANGE:
				{
					const notification = new FbsWebRtcTransport.IceStateChangeNotification();

					data!.body(notification);

					const iceState = fbsIceState2IceState(notification.iceState());

					this.#data.iceState = iceState;

					this.safeEmit('icestatechange', iceState);

					// Emit observer event.
					this.observer.safeEmit('icestatechange', iceState);

					break;
				}

				case Event.WEBRTCTRANSPORT_ICE_SELECTED_TUPLE_CHANGE:
				{
					const notification =
						new FbsWebRtcTransport.IceSelectedTupleChangeNotification();

					data!.body(notification);

					const iceSelectedTuple = parseTuple(notification.tuple()!);

					this.#data.iceSelectedTuple = iceSelectedTuple;

					this.safeEmit('iceselectedtuplechange', iceSelectedTuple);

					// Emit observer event.
					this.observer.safeEmit('iceselectedtuplechange', iceSelectedTuple);

					break;
				}

				case Event.WEBRTCTRANSPORT_DTLS_STATE_CHANGE:
				{
					const notification = new FbsWebRtcTransport.DtlsStateChangeNotification();

					data!.body(notification);

					const dtlsState = fbsDtlsState2DtlsState(notification.dtlsState());

					this.#data.dtlsState = dtlsState;

					if (dtlsState === 'connected')
					{
						this.#data.dtlsRemoteCert = notification.remoteCert()!;
					}

					this.safeEmit('dtlsstatechange', dtlsState);

					// Emit observer event.
					this.observer.safeEmit('dtlsstatechange', dtlsState);

					break;
				}

				case Event.TRANSPORT_SCTP_STATE_CHANGE:
				{
					const notification = new FbsTransport.SctpStateChangeNotification();

					data!.body(notification);

					const sctpState = fbsSctpState2StcpState(notification.sctpState());

					this.#data.sctpState = sctpState;

					this.safeEmit('sctpstatechange', sctpState);

					// Emit observer event.
					this.observer.safeEmit('sctpstatechange', sctpState);

					break;
				}

				case Event.TRANSPORT_TRACE:
				{
					const notification = new FbsTransport.TraceNotification();

					data!.body(notification);

					const trace = parseTransportTraceEventData(notification);

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

export function fbsIceState2IceState(fbsIceState: FbsIceState): IceState
{
	switch (fbsIceState)
	{
		case FbsIceState.NEW:
			return 'new';
		case FbsIceState.CONNECTED:
			return 'connected';
		case FbsIceState.COMPLETED:
			return 'completed';
		case FbsIceState.DISCONNECTED:
			return 'disconnected';
		case FbsIceState.CLOSED:
			return 'closed';
		default:
			throw new TypeError(`invalid SctpState: ${fbsIceState}`);
	}
}

export function fbsDtlsState2DtlsState(fbsDtlsState: FbsDtlsState): DtlsState
{
	switch (fbsDtlsState)
	{
		case FbsDtlsState.NEW:
			return 'new';
		case FbsDtlsState.CONNECTING:
			return 'connecting';
		case FbsDtlsState.CONNECTED:
			return 'connected';
		case FbsDtlsState.FAILED:
			return 'failed';
		case FbsDtlsState.CLOSED:
			return 'closed';
		default:
			throw new TypeError(`invalid SctpState: ${fbsDtlsState}`);
	}
}

export function parseWebRtcTransportDumpResponse(
	binary: FbsWebRtcTransport.DumpResponse
): WebRtcTransportDump
{
	// Retrieve BaseTransportDump.
	const baseTransportDump = parseBaseTransportDump(binary.base()!);
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

function createConnectRequest(
	{
		builder,
		dtlsParameters
	}:
	{
		builder : flatbuffers.Builder;
		dtlsParameters: DtlsParameters;
	}
): number
{
	// Serialize DtlsParameters. This can throw.
	const dtlsParametersOffset = serializeDtlsParameters(builder, dtlsParameters);

	// Create request.
	return FbsWebRtcTransport.ConnectRequest.createConnectRequest(
		builder,
		dtlsParametersOffset);
}

function parseGetStatsResponse(
	binary: FbsWebRtcTransport.GetStatsResponse
):WebRtcTransportStat
{
	const base = parseBaseTransportStats(binary.base()!);

	return {
		...base,
		type             : 'webrtc-transport',
		iceRole          : binary.iceRole()!,
		iceState         : binary.iceState() as IceState,
		iceSelectedTuple : binary.iceSelectedTuple() ?
			parseTuple(binary.iceSelectedTuple()!) :
			undefined,
		dtlsState : binary.dtlsState() as DtlsState
	};
}

function parseIceCandidate(binary: FbsWebRtcTransport.IceCandidate): IceCandidate
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

function parseIceParameters(binary: FbsWebRtcTransport.IceParameters): IceParameters
{
	return {
		usernameFragment : binary.usernameFragment()!,
		password         : binary.password()!,
		iceLite          : binary.iceLite()
	};
}

function parseDtlsParameters(binary: FbsWebRtcTransport.DtlsParameters): DtlsParameters
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

function serializeDtlsParameters(
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
			const fingerprintOffset = FbsWebRtcTransport.Fingerprint.createFingerprint(
				builder, algorithmOffset, valueOffset);

			fingerprints.push(fingerprintOffset);
		}

		const fingerprintsOffset = FbsWebRtcTransport.DtlsParameters.createFingerprintsVector(
			builder, fingerprints);
		const roleOffset = builder.createString(dtlsParameters.role);

		return FbsWebRtcTransport.DtlsParameters.createDtlsParameters(
			builder,
			fingerprintsOffset,
			roleOffset);
	}
	catch (error)
	{
		throw new TypeError(`${error}`);
	}
}
