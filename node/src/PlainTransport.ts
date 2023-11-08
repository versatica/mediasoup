import * as flatbuffers from 'flatbuffers';
import { Logger } from './Logger';
import {
	parseSctpState,
	BaseTransportDump,
	BaseTransportStats,
	parseTuple,
	parseBaseTransportDump,
	parseBaseTransportStats,
	parseTransportTraceEventData,
	Transport,
	TransportListenInfo,
	TransportListenIp,
	TransportTuple,
	TransportEvents,
	TransportObserverEvents,
	TransportConstructorOptions,
	SctpState
} from './Transport';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import {
	parseSrtpParameters,
	serializeSrtpParameters,
	SrtpParameters,
	SrtpCryptoSuite
} from './SrtpParameters';
import { AppData, Either } from './types';
import { Event, Notification } from './fbs/notification';
import * as FbsRequest from './fbs/request';
import * as FbsTransport from './fbs/transport';
import * as FbsPlainTransport from './fbs/plain-transport';

type PlainTransportListenInfo =
{
	/**
	 * Listening info.
	 */
	listenInfo: TransportListenInfo;

	/**
	 * Optional listening info for RTCP.
	 */
	rtcpListenInfo?: TransportListenInfo;
};

type PlainTransportListenIp =
{
	/**
	 * Listening IP address.
	 */
	listenIp: TransportListenIp | string;

	/**
	 * Fixed port to listen on instead of selecting automatically from Worker's port
	 * range.
	 */
	port?: number;
};

type PlainTransportListen = Either<PlainTransportListenInfo, PlainTransportListenIp>;

export type PlainTransportOptions<PlainTransportAppData extends AppData = AppData> =
{
	/**
	 * Use RTCP-mux (RTP and RTCP in the same port). Default true.
	 */
	rtcpMux?: boolean;

	/**
	 * Whether remote IP:port should be auto-detected based on first RTP/RTCP
	 * packet received. If enabled, connect() method must not be called unless
	 * SRTP is enabled. If so, it must be called with just remote SRTP parameters.
	 * Default false.
	 */
	comedia?: boolean;

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
	 * Enable SRTP. For this to work, connect() must be called
	 * with remote SRTP parameters. Default false.
	 */
	enableSrtp?: boolean;

	/**
	 * The SRTP crypto suite to be used if enableSrtp is set. Default
	 * 'AES_CM_128_HMAC_SHA1_80'.
	 */
	srtpCryptoSuite?: SrtpCryptoSuite;

	/**
	 * Custom application data.
	 */
	appData?: PlainTransportAppData;
} & PlainTransportListen;

export type PlainTransportStat = BaseTransportStats &
{
	type: string;
	rtcpMux: boolean;
	comedia: boolean;
	tuple: TransportTuple;
	rtcpTuple?: TransportTuple;
};

export type PlainTransportEvents = TransportEvents &
{
	tuple: [TransportTuple];
	rtcptuple: [TransportTuple];
	sctpstatechange: [SctpState];
};

export type PlainTransportObserverEvents = TransportObserverEvents &
{
	tuple: [TransportTuple];
	rtcptuple: [TransportTuple];
	sctpstatechange: [SctpState];	
};

type PlainTransportConstructorOptions<PlainTransportAppData> =
	TransportConstructorOptions<PlainTransportAppData> &
	{
		data: PlainTransportData;
	};

export type PlainTransportData =
{
	rtcpMux?: boolean;
	comedia?: boolean;
	tuple: TransportTuple;
	rtcpTuple?: TransportTuple;
	sctpParameters?: SctpParameters;
	sctpState?: SctpState;
	srtpParameters?: SrtpParameters;
};

type PlainTransportDump = BaseTransportDump &
{
	rtcpMux: boolean;
	comedia: boolean;
	tuple: TransportTuple;
	rtcpTuple?: TransportTuple;
	srtpParameters?: SrtpParameters;
};

const logger = new Logger('PlainTransport');

export class PlainTransport<PlainTransportAppData extends AppData = AppData>
	extends Transport<PlainTransportAppData, PlainTransportEvents, PlainTransportObserverEvents>
{
	// PlainTransport data.
	readonly #data: PlainTransportData;

	/**
	 * @private
	 */
	constructor(options: PlainTransportConstructorOptions<PlainTransportAppData>)
	{
		super(options);

		logger.debug('constructor()');

		const { data } = options;

		this.#data =
		{
			rtcpMux        : data.rtcpMux,
			comedia        : data.comedia,
			tuple          : data.tuple,
			rtcpTuple      : data.rtcpTuple,
			sctpParameters : data.sctpParameters,
			sctpState      : data.sctpState,
			srtpParameters : data.srtpParameters
		};

		this.handleWorkerNotifications();
	}

	/**
	 * Transport tuple.
	 */
	get tuple(): TransportTuple
	{
		return this.#data.tuple;
	}

	/**
	 * Transport RTCP tuple.
	 */
	get rtcpTuple(): TransportTuple | undefined
	{
		return this.#data.rtcpTuple;
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
	 * SRTP parameters.
	 */
	get srtpParameters(): SrtpParameters | undefined
	{
		return this.#data.srtpParameters;
	}

	/**
	 * Close the PlainTransport.
	 *
	 * @override
	 */
	close(): void
	{
		if (this.closed)
		{
			return;
		}

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

		if (this.#data.sctpState)
		{
			this.#data.sctpState = 'closed';
		}

		super.routerClosed();
	}

	/**
	 * Dump Transport.
	 */
	async dump(): Promise<PlainTransportDump>
	{
		logger.debug('dump()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_DUMP,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsPlainTransport.DumpResponse();

		response.body(data);

		return parsePlainTransportDumpResponse(data);
	}

	/**
	 * Get PlainTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<PlainTransportStat[]>
	{
		logger.debug('getStats()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_GET_STATS,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsPlainTransport.GetStatsResponse();

		response.body(data);

		return [ parseGetStatsResponse(data) ];
	}

	/**
	 * Provide the PlainTransport remote parameters.
	 *
	 * @override
	 */
	async connect(
		{
			ip,
			port,
			rtcpPort,
			srtpParameters
		}:
		{
			ip?: string;
			port?: number;
			rtcpPort?: number;
			srtpParameters?: SrtpParameters;
		}
	): Promise<void>
	{
		logger.debug('connect()');

		const requestOffset = createConnectRequest({
			builder : this.channel.bufferBuilder,
			ip,
			port,
			rtcpPort,
			srtpParameters
		});

		// Wait for response.
		const response = await this.channel.request(
			FbsRequest.Method.PLAINTRANSPORT_CONNECT,
			FbsRequest.Body.PlainTransport_ConnectRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsPlainTransport.ConnectResponse();

		response.body(data);

		// Update data.
		if (data.tuple())
		{
			this.#data.tuple = parseTuple(data.tuple()!);
		}

		if (data.rtcpTuple())
		{
			this.#data.rtcpTuple = parseTuple(data.rtcpTuple()!);
		}

		if (data.srtpParameters())
		{
			this.#data.srtpParameters = parseSrtpParameters(
				data.srtpParameters()!);
		}
	}

	private handleWorkerNotifications(): void
	{
		this.channel.on(this.internal.transportId, (event: Event, data?: Notification) =>
		{
			switch (event)
			{
				case Event.PLAINTRANSPORT_TUPLE:
				{
					const notification = new FbsPlainTransport.TupleNotification();

					data!.body(notification);

					const tuple = parseTuple(notification.tuple()!);

					this.#data.tuple = tuple;

					this.safeEmit('tuple', tuple);

					// Emit observer event.
					this.observer.safeEmit('tuple', tuple);

					break;
				}

				case Event.PLAINTRANSPORT_RTCP_TUPLE:
				{
					const notification = new FbsPlainTransport.RtcpTupleNotification();

					data!.body(notification);

					const rtcpTuple = parseTuple(notification.tuple()!);

					this.#data.rtcpTuple = rtcpTuple;

					this.safeEmit('rtcptuple', rtcpTuple);

					// Emit observer event.
					this.observer.safeEmit('rtcptuple', rtcpTuple);

					break;
				}

				case Event.TRANSPORT_SCTP_STATE_CHANGE:
				{
					const notification = new FbsTransport.SctpStateChangeNotification();

					data!.body(notification);

					const sctpState = parseSctpState(notification.sctpState());

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

export function parsePlainTransportDumpResponse(
	binary: FbsPlainTransport.DumpResponse
): PlainTransportDump
{
	// Retrieve BaseTransportDump.
	const baseTransportDump = parseBaseTransportDump(binary.base()!);
	// Retrieve RTP Tuple.
	const tuple = parseTuple(binary.tuple()!);

	// Retrieve RTCP Tuple.
	let rtcpTuple: TransportTuple | undefined;

	if (binary.rtcpTuple())
	{
		rtcpTuple = parseTuple(binary.rtcpTuple()!);
	}

	// Retrieve SRTP Parameters.
	let srtpParameters: SrtpParameters | undefined;

	if (binary.srtpParameters())
	{
		srtpParameters = parseSrtpParameters(binary.srtpParameters()!);
	}

	return {
		...baseTransportDump,
		rtcpMux        : binary.rtcpMux(),
		comedia        : binary.comedia(),
		tuple          : tuple,
		rtcpTuple      : rtcpTuple,
		srtpParameters : srtpParameters
	};
}

function parseGetStatsResponse(
	binary: FbsPlainTransport.GetStatsResponse
):PlainTransportStat
{
	const base = parseBaseTransportStats(binary.base()!);

	return {
		...base,
		type      : 'plain-rtp-transport',
		rtcpMux   : binary.rtcpMux(),
		comedia   : binary.comedia(),
		tuple     : parseTuple(binary.tuple()!),
		rtcpTuple : binary.rtcpTuple() ?
			parseTuple(binary.rtcpTuple()!) :
			undefined
	};
}

function createConnectRequest(
	{
		builder,
		ip,
		port,
		rtcpPort,
		srtpParameters
	}:
	{
		builder : flatbuffers.Builder;
		ip?: string;
		port?: number;
		rtcpPort?: number;
		srtpParameters?: SrtpParameters;
	}
): number
{
	let ipOffset = 0;
	let srtpParametersOffset = 0;

	if (ip)
	{
		ipOffset = builder.createString(ip);
	}

	// Serialize SrtpParameters.
	if (srtpParameters)
	{
		srtpParametersOffset = serializeSrtpParameters(builder, srtpParameters);
	}

	// Create PlainTransportConnectData.
	FbsPlainTransport.ConnectRequest.startConnectRequest(builder);
	FbsPlainTransport.ConnectRequest.addIp(builder, ipOffset);

	if (typeof port === 'number')
	{
		FbsPlainTransport.ConnectRequest.addPort(builder, port);
	}
	if (typeof rtcpPort === 'number')
	{
		FbsPlainTransport.ConnectRequest.addRtcpPort(builder, rtcpPort);
	}
	if (srtpParameters)
	{
		FbsPlainTransport.ConnectRequest.addSrtpParameters(
			builder, srtpParametersOffset
		);
	}

	return FbsPlainTransport.ConnectRequest.endConnectRequest(builder);
}
