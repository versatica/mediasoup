import * as flatbuffers from 'flatbuffers';
import { Logger } from './Logger';
import * as ortc from './ortc';
import {
	BaseTransportDump,
	BaseTransportStats,
	parseBaseTransportDump,
	parseBaseTransportStats,
	parseSctpState,
	parseTuple,
	parseTransportTraceEventData,
	Transport,
	TransportListenInfo,
	TransportListenIp,
	TransportTuple,
	TransportEvents,
	TransportObserverEvents,
	TransportConstructorOptions,
	SctpState,
} from './Transport';
import { Consumer, ConsumerType } from './Consumer';
import { Producer } from './Producer';
import {
	RtpParameters,
	serializeRtpEncodingParameters,
	serializeRtpParameters,
} from './RtpParameters';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import {
	parseSrtpParameters,
	serializeSrtpParameters,
	SrtpParameters,
} from './SrtpParameters';
import { AppData, Either } from './types';
import { generateUUIDv4 } from './utils';
import { MediaKind as FbsMediaKind } from './fbs/rtp-parameters/media-kind';
import * as FbsRtpParameters from './fbs/rtp-parameters';
import { Event, Notification } from './fbs/notification';
import * as FbsRequest from './fbs/request';
import * as FbsTransport from './fbs/transport';
import * as FbsPipeTransport from './fbs/pipe-transport';

type PipeTransportListenInfo = {
	/**
	 * Listening info.
	 */
	listenInfo: TransportListenInfo;
};

type PipeTransportListenIp = {
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

type PipeTransportListen = Either<
	PipeTransportListenInfo,
	PipeTransportListenIp
>;

export type PipeTransportOptions<
	PipeTransportAppData extends AppData = AppData,
> = {
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
	appData?: PipeTransportAppData;
} & PipeTransportListen;

export type PipeTransportStat = BaseTransportStats & {
	type: string;
	tuple: TransportTuple;
};

export type PipeConsumerOptions<ConsumerAppData> = {
	/**
	 * The id of the Producer to consume.
	 */
	producerId: string;

	/**
	 * Custom application data.
	 */
	appData?: ConsumerAppData;
};

export type PipeTransportEvents = TransportEvents & {
	sctpstatechange: [SctpState];
};

export type PipeTransportObserverEvents = TransportObserverEvents & {
	sctpstatechange: [SctpState];
};

type PipeTransportConstructorOptions<PipeTransportAppData> =
	TransportConstructorOptions<PipeTransportAppData> & {
		data: PipeTransportData;
	};

export type PipeTransportData = {
	tuple: TransportTuple;
	sctpParameters?: SctpParameters;
	sctpState?: SctpState;
	rtx: boolean;
	srtpParameters?: SrtpParameters;
};

export type PipeTransportDump = BaseTransportDump & {
	tuple: TransportTuple;
	rtx: boolean;
	srtpParameters?: SrtpParameters;
};

const logger = new Logger('PipeTransport');

export class PipeTransport<
	PipeTransportAppData extends AppData = AppData,
> extends Transport<
	PipeTransportAppData,
	PipeTransportEvents,
	PipeTransportObserverEvents
> {
	// PipeTransport data.
	readonly #data: PipeTransportData;

	/**
	 * @private
	 */
	constructor(options: PipeTransportConstructorOptions<PipeTransportAppData>) {
		super(options);

		logger.debug('constructor()');

		const { data } = options;

		this.#data = {
			tuple: data.tuple,
			sctpParameters: data.sctpParameters,
			sctpState: data.sctpState,
			rtx: data.rtx,
			srtpParameters: data.srtpParameters,
		};

		this.handleWorkerNotifications();
	}

	/**
	 * Transport tuple.
	 */
	get tuple(): TransportTuple {
		return this.#data.tuple;
	}

	/**
	 * SCTP parameters.
	 */
	get sctpParameters(): SctpParameters | undefined {
		return this.#data.sctpParameters;
	}

	/**
	 * SCTP state.
	 */
	get sctpState(): SctpState | undefined {
		return this.#data.sctpState;
	}

	/**
	 * SRTP parameters.
	 */
	get srtpParameters(): SrtpParameters | undefined {
		return this.#data.srtpParameters;
	}

	/**
	 * Close the PipeTransport.
	 *
	 * @override
	 */
	close(): void {
		if (this.closed) {
			return;
		}

		if (this.#data.sctpState) {
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
	routerClosed(): void {
		if (this.closed) {
			return;
		}

		if (this.#data.sctpState) {
			this.#data.sctpState = 'closed';
		}

		super.routerClosed();
	}

	/**
	 * Get PipeTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<PipeTransportStat[]> {
		logger.debug('getStats()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_GET_STATS,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsPipeTransport.GetStatsResponse();

		response.body(data);

		return [parseGetStatsResponse(data)];
	}

	/**
	 * Provide the PipeTransport remote parameters.
	 *
	 * @override
	 */
	async connect({
		ip,
		port,
		srtpParameters,
	}: {
		ip: string;
		port: number;
		srtpParameters?: SrtpParameters;
	}): Promise<void> {
		logger.debug('connect()');

		const requestOffset = createConnectRequest({
			builder: this.channel.bufferBuilder,
			ip,
			port,
			srtpParameters,
		});

		// Wait for response.
		const response = await this.channel.request(
			FbsRequest.Method.PIPETRANSPORT_CONNECT,
			FbsRequest.Body.PipeTransport_ConnectRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsPipeTransport.ConnectResponse();

		response.body(data);

		// Update data.
		if (data.tuple()) {
			this.#data.tuple = parseTuple(data.tuple()!);
		}
	}

	/**
	 * Create a pipe Consumer.
	 *
	 * @override
	 */
	async consume<ConsumerAppData extends AppData = AppData>({
		producerId,
		appData,
	}: PipeConsumerOptions<ConsumerAppData>): Promise<Consumer<ConsumerAppData>> {
		logger.debug('consume()');

		if (!producerId || typeof producerId !== 'string') {
			throw new TypeError('missing producerId');
		} else if (appData && typeof appData !== 'object') {
			throw new TypeError('if given, appData must be an object');
		}

		const producer = this.getProducerById(producerId);

		if (!producer) {
			throw Error(`Producer with id "${producerId}" not found`);
		}

		// This may throw.
		const rtpParameters = ortc.getPipeConsumerRtpParameters({
			consumableRtpParameters: producer.consumableRtpParameters,
			enableRtx: this.#data.rtx,
		});

		const consumerId = generateUUIDv4();

		const consumeRequestOffset = createConsumeRequest({
			builder: this.channel.bufferBuilder,
			consumerId,
			producer,
			rtpParameters,
		});

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_CONSUME,
			FbsRequest.Body.Transport_ConsumeRequest,
			consumeRequestOffset,
			this.internal.transportId
		);

		/* Decode Response. */
		const consumeResponse = new FbsTransport.ConsumeResponse();

		response.body(consumeResponse);

		const status = consumeResponse.unpack();

		const data = {
			producerId,
			kind: producer.kind,
			rtpParameters,
			type: 'pipe' as ConsumerType,
		};

		const consumer = new Consumer<ConsumerAppData>({
			internal: {
				...this.internal,
				consumerId,
			},
			data,
			channel: this.channel,
			appData,
			paused: status.paused,
			producerPaused: status.producerPaused,
		});

		this.consumers.set(consumer.id, consumer);
		consumer.on('@close', () => this.consumers.delete(consumer.id));
		consumer.on('@producerclose', () => this.consumers.delete(consumer.id));

		// Emit observer event.
		this.observer.safeEmit('newconsumer', consumer);

		return consumer;
	}

	private handleWorkerNotifications(): void {
		this.channel.on(
			this.internal.transportId,
			(event: Event, data?: Notification) => {
				switch (event) {
					case Event.TRANSPORT_SCTP_STATE_CHANGE: {
						const notification = new FbsTransport.SctpStateChangeNotification();

						data!.body(notification);

						const sctpState = parseSctpState(notification.sctpState());

						this.#data.sctpState = sctpState;

						this.safeEmit('sctpstatechange', sctpState);

						// Emit observer event.
						this.observer.safeEmit('sctpstatechange', sctpState);

						break;
					}

					case Event.TRANSPORT_TRACE: {
						const notification = new FbsTransport.TraceNotification();

						data!.body(notification);

						const trace = parseTransportTraceEventData(notification);

						this.safeEmit('trace', trace);

						// Emit observer event.
						this.observer.safeEmit('trace', trace);

						break;
					}

					default: {
						logger.error('ignoring unknown event "%s"', event);
					}
				}
			}
		);
	}
}

/*
 * flatbuffers helpers.
 */

export function parsePipeTransportDumpResponse(
	binary: FbsPipeTransport.DumpResponse
): PipeTransportDump {
	// Retrieve BaseTransportDump.
	const baseTransportDump = parseBaseTransportDump(binary.base()!);
	// Retrieve RTP Tuple.
	const tuple = parseTuple(binary.tuple()!);

	// Retrieve SRTP Parameters.
	let srtpParameters: SrtpParameters | undefined;

	if (binary.srtpParameters()) {
		srtpParameters = parseSrtpParameters(binary.srtpParameters()!);
	}

	return {
		...baseTransportDump,
		tuple: tuple,
		rtx: binary.rtx(),
		srtpParameters: srtpParameters,
	};
}

function parseGetStatsResponse(
	binary: FbsPipeTransport.GetStatsResponse
): PipeTransportStat {
	const base = parseBaseTransportStats(binary.base()!);

	return {
		...base,
		type: 'pipe-transport',
		tuple: parseTuple(binary.tuple()!),
	};
}

function createConsumeRequest({
	builder,
	consumerId,
	producer,
	rtpParameters,
}: {
	builder: flatbuffers.Builder;
	consumerId: string;
	producer: Producer;
	rtpParameters: RtpParameters;
}): number {
	// Build the request.
	const producerIdOffset = builder.createString(producer.id);
	const consumerIdOffset = builder.createString(consumerId);
	const rtpParametersOffset = serializeRtpParameters(builder, rtpParameters);
	let consumableRtpEncodingsOffset: number | undefined;

	if (producer.consumableRtpParameters.encodings) {
		consumableRtpEncodingsOffset = serializeRtpEncodingParameters(
			builder,
			producer.consumableRtpParameters.encodings
		);
	}

	const ConsumeRequest = FbsTransport.ConsumeRequest;

	// Create Consume Request.
	ConsumeRequest.startConsumeRequest(builder);
	ConsumeRequest.addConsumerId(builder, consumerIdOffset);
	ConsumeRequest.addProducerId(builder, producerIdOffset);
	ConsumeRequest.addKind(
		builder,
		producer.kind === 'audio' ? FbsMediaKind.AUDIO : FbsMediaKind.VIDEO
	);
	ConsumeRequest.addRtpParameters(builder, rtpParametersOffset);
	ConsumeRequest.addType(builder, FbsRtpParameters.Type.PIPE);

	if (consumableRtpEncodingsOffset) {
		ConsumeRequest.addConsumableRtpEncodings(
			builder,
			consumableRtpEncodingsOffset
		);
	}

	return ConsumeRequest.endConsumeRequest(builder);
}

function createConnectRequest({
	builder,
	ip,
	port,
	srtpParameters,
}: {
	builder: flatbuffers.Builder;
	ip?: string;
	port?: number;
	srtpParameters?: SrtpParameters;
}): number {
	let ipOffset = 0;
	let srtpParametersOffset = 0;

	if (ip) {
		ipOffset = builder.createString(ip);
	}

	// Serialize SrtpParameters.
	if (srtpParameters) {
		srtpParametersOffset = serializeSrtpParameters(builder, srtpParameters);
	}

	// Create PlainTransportConnectData.
	FbsPipeTransport.ConnectRequest.startConnectRequest(builder);
	FbsPipeTransport.ConnectRequest.addIp(builder, ipOffset);

	if (typeof port === 'number') {
		FbsPipeTransport.ConnectRequest.addPort(builder, port);
	}
	if (srtpParameters) {
		FbsPipeTransport.ConnectRequest.addSrtpParameters(
			builder,
			srtpParametersOffset
		);
	}

	return FbsPipeTransport.ConnectRequest.endConnectRequest(builder);
}
