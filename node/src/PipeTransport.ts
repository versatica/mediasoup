import { v4 as uuidv4 } from 'uuid';
import * as flatbuffers from 'flatbuffers';
import { Logger } from './Logger';
import * as ortc from './ortc';
import {
	BaseTransportDump,
	parseTuple,
	parseBaseTransportDump,
	Transport,
	TransportListenIp,
	TransportTuple,
	TransportTraceEventData,
	TransportEvents,
	TransportObserverEvents,
	TransportConstructorOptions,
	SctpState
} from './Transport';
import { Consumer, ConsumerType } from './Consumer';
import { Producer } from './Producer';
import { RtpParameters, serializeRtpEncodingParameters, serializeRtpParameters } from './RtpParameters';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import { parseSrtpParameters, serializeSrtpParameters, SrtpParameters } from './SrtpParameters';
import { MediaKind as FbsMediaKind } from './fbs/fbs/rtp-parameters/media-kind';
import * as FbsRequest from './fbs/request_generated';
import * as FbsResponse from './fbs/response_generated';
import * as FbsTransport from './fbs/transport_generated';

export type PipeTransportOptions =
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
	 * Default 268435456.
	 */
	maxSctpMessageSize?: number;

	/**
	 * Maximum SCTP send buffer used by DataConsumers.
	 * Default 268435456.
	 */
	sctpSendBufferSize?: number;

	/**
	 * Enable RTX and NACK for RTP retransmission. Useful if both Routers are
	 * located in different hosts and there is packet lost in the link. For this
	 * to work, both PipeTransports must enable this setting. Default false.
	 */
	enableRtx?: boolean;

	/**
	 * Enable SRTP. Useful to protect the RTP and RTCP traffic if both Routers
	 * are located in different hosts. For this to work, connect() must be called
	 * with remote SRTP parameters. Default false.
	 */
	enableSrtp?: boolean;

	/**
	 * Custom application data.
	 */
	appData?: Record<string, unknown>;
};

export type PipeTransportStat =
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
	// PipeTransport specific.
	tuple: TransportTuple;
};

export type PipeConsumerOptions =
{
	/**
	 * The id of the Producer to consume.
	 */
	producerId: string;

	/**
	 * Custom application data.
	 */
	appData?: Record<string, unknown>;
};

export type PipeTransportEvents = TransportEvents &
{
	sctpstatechange: [SctpState];
};

export type PipeTransportObserverEvents = TransportObserverEvents &
{
	sctpstatechange: [SctpState];
};

type PipeTransportConstructorOptions = TransportConstructorOptions &
{
	data: PipeTransportData;
};

export type PipeTransportData =
{
	tuple: TransportTuple;
	sctpParameters?: SctpParameters;
	sctpState?: SctpState;
	rtx: boolean;
	srtpParameters?: SrtpParameters;
};

export type PipeTransportDump = BaseTransportDump &
{
	tuple: TransportTuple;
	rtx: boolean;
	srtpParameters?: SrtpParameters;
};

const logger = new Logger('PipeTransport');

export class PipeTransport
	extends Transport<PipeTransportEvents, PipeTransportObserverEvents>
{
	// PipeTransport data.
	readonly #data: PipeTransportData;

	/**
	 * @private
	 */
	constructor(options: PipeTransportConstructorOptions)
	{
		super(options);

		logger.debug('constructor()');

		const { data } = options;

		this.#data =
		{
			tuple          : data.tuple,
			sctpParameters : data.sctpParameters,
			sctpState      : data.sctpState,
			rtx            : data.rtx,
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
	 * Close the PipeTransport.
	 *
	 * @override
	 */
	close(): void
	{
		if (this.closed)
			return;

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

		if (this.#data.sctpState)
			this.#data.sctpState = 'closed';

		super.routerClosed();
	}

	/**
	 * Get PipeTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<PipeTransportStat[]>
	{
		logger.debug('getStats()');

		const response = await this.channel.request(
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
	 * Provide the PipeTransport remote parameters.
	 *
	 * @override
	 */
	async connect(
		{
			ip,
			port,
			srtpParameters
		}:
		{
			ip: string;
			port: number;
			srtpParameters?: SrtpParameters;
		}
	): Promise<void>
	{
		logger.debug('connect()');

		const builder = this.channel.bufferBuilder;

		const connectData = createConnectRequest({
			builder,
			ip,
			port,
			srtpParameters
		});

		const requestOffset = FbsTransport.ConnectRequest.createConnectRequest(
			builder,
			FbsTransport.ConnectData.ConnectPipeTransportData,
			connectData
		);

		// Wait for response.
		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_CONNECT,
			FbsRequest.Body.FBS_Transport_ConnectRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode the response. */
		const connectResponse = new FbsTransport.ConnectResponse();

		response.body(connectResponse);

		const data =
			new FbsTransport.ConnectPlainTransportResponse();

		connectResponse.data(data);

		// Update data.
		if (data.tuple())
			this.#data.tuple = parseTuple(data.tuple()!);
	}

	/**
	 * Create a pipe Consumer.
	 *
	 * @override
	 */
	async consume({ producerId, appData }: PipeConsumerOptions): Promise<Consumer>
	{
		logger.debug('consume()');

		if (!producerId || typeof producerId !== 'string')
			throw new TypeError('missing producerId');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		const producer = this.getProducerById(producerId);

		if (!producer)
			throw Error(`Producer with id "${producerId}" not found`);

		// This may throw.
		const rtpParameters = ortc.getPipeConsumerRtpParameters(
			producer.consumableRtpParameters, this.#data.rtx);

		const consumerId = uuidv4();

		const consumeRequestOffset = createConsumeRequest({
			builder : this.channel.bufferBuilder,
			consumerId,
			producer,
			rtpParameters
		});

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_CONSUME,
			FbsRequest.Body.FBS_Transport_ConsumeRequest,
			consumeRequestOffset,
			this.internal.transportId
		);

		/* Decode the response. */
		const consumeResponse = new FbsResponse.ConsumeResponse();

		response.body(consumeResponse);

		const status = consumeResponse.unpack();

		const data =
		{
			producerId,
			kind : producer.kind,
			rtpParameters,
			type : 'pipe' as ConsumerType
		};

		const consumer = new Consumer(
			{
				internal :
				{
					...this.internal,
					consumerId
				},
				data,
				channel        : this.channel,
				payloadChannel : this.payloadChannel,
				appData,
				paused         : status.paused,
				producerPaused : status.producerPaused
			});

		this.consumers.set(consumer.id, consumer);
		consumer.on('@close', () => this.consumers.delete(consumer.id));
		consumer.on('@producerclose', () => this.consumers.delete(consumer.id));

		// Emit observer event.
		this.observer.safeEmit('newconsumer', consumer);

		return consumer;
	}

	private handleWorkerNotifications(): void
	{
		this.channel.on(this.internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
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

/*
 * flatbuffers helpers
 */

export function parsePipeTransportDump(
	binary: FbsTransport.PipeTransportDump
): PipeTransportDump
{
	// Retrieve BaseTransportDump.
	const fbsBaseTransportDump = new FbsTransport.BaseTransportDump();

	binary.base()!.data(fbsBaseTransportDump);
	const baseTransportDump = parseBaseTransportDump(fbsBaseTransportDump);

	// Retrieve RTP Tuple.
	const tuple = parseTuple(binary.tuple()!);

	// Retrieve SRTP Parameters.
	let srtpParameters: SrtpParameters | undefined;

	if (binary.srtpParameters())
	{
		srtpParameters = parseSrtpParameters(binary.srtpParameters()!);
	}

	return {
		...baseTransportDump,
		tuple          : tuple,
		rtx            : binary.rtx(),
		srtpParameters : srtpParameters
	};
}

function createConsumeRequest({
	builder,
	consumerId,
	producer,
	rtpParameters
} : {
	builder: flatbuffers.Builder;
	consumerId: string;
	producer: Producer;
	rtpParameters: RtpParameters;
}): number
{
	// Build the request.
	const producerIdOffset = builder.createString(producer.id);
	const consumerIdOffset = builder.createString(consumerId);
	const rtpParametersOffset = serializeRtpParameters(builder, rtpParameters);
	let consumableRtpEncodingsOffset: number | undefined;

	if (producer.consumableRtpParameters.encodings)
	{
		consumableRtpEncodingsOffset = serializeRtpEncodingParameters(
			builder, producer.consumableRtpParameters.encodings
		);
	}

	const ConsumeRequest = FbsRequest.ConsumeRequest;

	// Create Consume Request.
	ConsumeRequest.startConsumeRequest(builder);
	ConsumeRequest.addConsumerId(builder, consumerIdOffset);
	ConsumeRequest.addProducerId(builder, producerIdOffset);
	ConsumeRequest.addKind(
		builder, producer.kind === 'audio' ? FbsMediaKind.AUDIO : FbsMediaKind.VIDEO);
	ConsumeRequest.addRtpParameters(builder, rtpParametersOffset);
	ConsumeRequest.addType(builder, FbsTransport.Type.PIPE);

	if (consumableRtpEncodingsOffset)
		ConsumeRequest.addConsumableRtpEncodings(builder, consumableRtpEncodingsOffset);

	return ConsumeRequest.endConsumeRequest(builder);
}

function createConnectRequest(
	{
		builder,
		ip,
		port,
		srtpParameters
	}:
	{
		builder: flatbuffers.Builder;
		ip?: string;
		port?: number;
		srtpParameters?: SrtpParameters;
	}
): number
{
	try
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
		FbsTransport.ConnectPipeTransportData.startConnectPipeTransportData(builder);
		FbsTransport.ConnectPipeTransportData.addIp(builder, ipOffset);

		if (typeof port === 'number')
			FbsTransport.ConnectPipeTransportData.addPort(builder, port);
		if (srtpParameters)
			FbsTransport.ConnectPipeTransportData.addSrtpParameters(
				builder, srtpParametersOffset
			);

		return FbsTransport.ConnectPipeTransportData.endConnectPipeTransportData(builder);
	}
	catch (error)
	{
		throw new TypeError(`${error}`);
	}
}
