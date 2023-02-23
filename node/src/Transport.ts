import { v4 as uuidv4 } from 'uuid';
import * as flatbuffers from 'flatbuffers';
import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import * as utils from './utils';
import * as ortc from './ortc';
import { Channel } from './Channel';
import { RouterInternal } from './Router';
import { WebRtcTransportData } from './WebRtcTransport';
import { PlainTransportData } from './PlainTransport';
import { PipeTransportData } from './PipeTransport';
import { DirectTransportData } from './DirectTransport';
import { Producer, ProducerOptions } from './Producer';
import { Consumer, ConsumerLayers, ConsumerOptions, ConsumerType } from './Consumer';
import {
	DataProducer,
	DataProducerOptions,
	DataProducerType,
	parseDataProducerDump
} from './DataProducer';
import {
	DataConsumer,
	DataConsumerOptions,
	DataConsumerType,
	parseDataConsumerDump
} from './DataConsumer';
import { MediaKind, RtpCapabilities, RtpParameters, serializeRtpEncodingParameters, serializeRtpParameters } from './RtpParameters';
import { parseSctpParametersDump, serializeSctpStreamParameters, SctpParameters, SctpStreamParameters } from './SctpParameters';
import * as FbsRequest from './fbs/request_generated';
import * as FbsResponse from './fbs/response_generated';
import { MediaKind as FbsMediaKind } from './fbs/fbs/rtp-parameters/media-kind';
import * as FbsConsumer from './fbs/consumer_generated';
import * as FbsDataConsumer from './fbs/dataConsumer_generated';
import * as FbsDataProducer from './fbs/dataProducer_generated';
import * as FbsTransport from './fbs/transport_generated';
import * as FbsRouter from './fbs/router_generated';
import { SctpState as FbsSctpState } from './fbs/fbs/sctp-association/sctp-state';

export type TransportListenIp =
{
	/**
	 * Listening IPv4 or IPv6.
	 */
	ip: string;

	/**
	 * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
	 * private IP).
	 */
	announcedIp?: string;
};

/**
 * Transport protocol.
 */
export type TransportProtocol = 'udp' | 'tcp';

export type TransportTuple =
{
	localIp: string;
	localPort: number;
	remoteIp?: string;
	remotePort?: number;
	protocol: TransportProtocol;
};

/**
 * Valid types for 'trace' event.
 */
export type TransportTraceEventType = 'probation' | 'bwe';

/**
 * 'trace' event data.
 */
export type TransportTraceEventData =
{
	/**
	 * Trace type.
	 */
	type: TransportTraceEventType;

	/**
	 * Event timestamp.
	 */
	timestamp: number;

	/**
	 * Event direction.
	 */
	direction: 'in' | 'out';

	/**
	 * Per type information.
	 */
	info: any;
};

export type SctpState = 'new' | 'connecting' | 'connected' | 'failed' | 'closed';

export type TransportEvents = 
{ 
	routerclose: [];
	listenserverclose: [];
	trace: [TransportTraceEventData];
	// Private events.
	'@close': [];
	'@newproducer': [Producer];
	'@producerclose': [Producer];
	'@newdataproducer': [DataProducer];
	'@dataproducerclose': [DataProducer];
	'@listenserverclose': [];
};

export type TransportObserverEvents =
{
	close: [];
	newproducer: [Producer];
	newconsumer: [Consumer];
	newdataproducer: [DataProducer];
	newdataconsumer: [DataConsumer];
	trace: [TransportTraceEventData];
};

export type TransportConstructorOptions =
{
	internal: TransportInternal;
	data: TransportData;
	channel: Channel;
	appData?: Record<string, unknown>;
	getRouterRtpCapabilities: () => RtpCapabilities;
	getProducerById: (producerId: string) => Producer | undefined;
	getDataProducerById: (dataProducerId: string) => DataProducer | undefined;
};

export type TransportInternal = RouterInternal &
{
	transportId: string;
};

export type BaseTransportDump = {
	id : string;
	direct : boolean;
	producerIds : string[];
	consumerIds : string[];
	mapSsrcConsumerId : { key: number; value: string }[];
	mapRtxSsrcConsumerId : { key: number; value: string }[];
	recvRtpHeaderExtensions : { key: string; value: number }[];
	rtpListener: RtpListenerDump;
	maxMessageSize: number;
	dataProducerIds : string[];
	dataConsumerIds : string[];
	sctpParameters? : SctpParameters;
	sctpState? : SctpState;
	sctpListener?: SctpListenerDump;
	traceEventTypes? : string[];
};

export type BaseTransportStats = {
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
};

type TransportData =
  | WebRtcTransportData
  | PlainTransportData
  | PipeTransportData
  | DirectTransportData;

type RtpListenerDump = {
	ssrcTable : {key: number; value: string}[];
	midTable : {key: number; value: string}[];
	ridTable : {key: number; value: string}[];
};

type SctpListenerDump = {
	streamIdTable : {key: number; value: string}[];
};

const logger = new Logger('Transport');

export class Transport<Events extends TransportEvents = TransportEvents,
	ObserverEvents extends TransportObserverEvents = TransportObserverEvents>
	extends EnhancedEventEmitter<Events>
{
	// Internal data.
	protected readonly internal: TransportInternal;

	// Transport data. This is set by the subclass.
	readonly #data: TransportData;

	// Channel instance.
	protected readonly channel: Channel;

	// Close flag.
	#closed = false;

	// Custom app data.
	readonly #appData: Record<string, unknown>;

	// Method to retrieve Router RTP capabilities.
	readonly #getRouterRtpCapabilities: () => RtpCapabilities;

	// Method to retrieve a Producer.
	protected readonly getProducerById: (producerId: string) => Producer | undefined;

	// Method to retrieve a DataProducer.
	protected readonly getDataProducerById:
		(dataProducerId: string) => DataProducer | undefined;

	// Producers map.
	readonly #producers: Map<string, Producer> = new Map();

	// Consumers map.
	protected readonly consumers: Map<string, Consumer> = new Map();

	// DataProducers map.
	protected readonly dataProducers: Map<string, DataProducer> = new Map();

	// DataConsumers map.
	protected readonly dataConsumers: Map<string, DataConsumer> = new Map();

	// RTCP CNAME for Producers.
	#cnameForProducers?: string;

	// Next MID for Consumers. It's converted into string when used.
	#nextMidForConsumers = 0;

	// Buffer with available SCTP stream ids.
	#sctpStreamIds?: Buffer;

	// Next SCTP stream id.
	#nextSctpStreamId = 0;

	// Observer instance.
	readonly #observer = new EnhancedEventEmitter<ObserverEvents>();

	/**
	 * @private
	 * @interface
	 */
	constructor(
		{
			internal,
			data,
			channel,
			appData,
			getRouterRtpCapabilities,
			getProducerById,
			getDataProducerById
		}: TransportConstructorOptions
	)
	{
		super();

		logger.debug('constructor()');

		this.internal = internal;
		this.#data = data;
		this.channel = channel;
		this.#appData = appData || {};
		this.#getRouterRtpCapabilities = getRouterRtpCapabilities;
		this.getProducerById = getProducerById;
		this.getDataProducerById = getDataProducerById;
	}

	/**
	 * Transport id.
	 */
	get id(): string
	{
		return this.internal.transportId;
	}

	/**
	 * Whether the Transport is closed.
	 */
	get closed(): boolean
	{
		return this.#closed;
	}

	/**
	 * App custom data.
	 */
	get appData(): Record<string, unknown>
	{
		return this.#appData;
	}

	/**
	 * Invalid setter.
	 */
	set appData(appData: Record<string, unknown>) // eslint-disable-line no-unused-vars
	{
		throw new Error('cannot override appData object');
	}

	/**
	 * Observer.
	 */
	get observer(): EnhancedEventEmitter<ObserverEvents>
	{
		return this.#observer;
	}

	/**
	 * @private
	 * Just for testing purposes.
	 */
	get channelForTesting(): Channel
	{
		return this.channel;
	}

	/**
	 * Close the Transport.
	 */
	close(): void
	{
		if (this.#closed)
		{
			return;
		}

		logger.debug('close()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.channel.removeAllListeners(this.internal.transportId);

		/* Build Request. */

		const requestOffset = new FbsRouter.CloseTransportRequestT(
			this.internal.transportId
		).pack(this.channel.bufferBuilder);

		this.channel.request(
			FbsRequest.Method.ROUTER_CLOSE_TRANSPORT,
			FbsRequest.Body.FBS_Router_CloseTransportRequest,
			requestOffset,
			this.internal.routerId
		).catch(() => {});

		// Close every Producer.
		for (const producer of this.#producers.values())
		{
			producer.transportClosed();

			// Must tell the Router.
			this.emit('@producerclose', producer);
		}
		this.#producers.clear();

		// Close every Consumer.
		for (const consumer of this.consumers.values())
		{
			consumer.transportClosed();
		}
		this.consumers.clear();

		// Close every DataProducer.
		for (const dataProducer of this.dataProducers.values())
		{
			dataProducer.transportClosed();

			// Must tell the Router.
			this.emit('@dataproducerclose', dataProducer);
		}
		this.dataProducers.clear();

		// Close every DataConsumer.
		for (const dataConsumer of this.dataConsumers.values())
		{
			dataConsumer.transportClosed();
		}
		this.dataConsumers.clear();

		this.emit('@close');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @virtual
	 */
	routerClosed(): void
	{
		if (this.#closed)
		{
			return;
		}

		logger.debug('routerClosed()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.channel.removeAllListeners(this.internal.transportId);

		// Close every Producer.
		for (const producer of this.#producers.values())
		{
			producer.transportClosed();

			// NOTE: No need to tell the Router since it already knows (it has
			// been closed in fact).
		}
		this.#producers.clear();

		// Close every Consumer.
		for (const consumer of this.consumers.values())
		{
			consumer.transportClosed();
		}
		this.consumers.clear();

		// Close every DataProducer.
		for (const dataProducer of this.dataProducers.values())
		{
			dataProducer.transportClosed();

			// NOTE: No need to tell the Router since it already knows (it has
			// been closed in fact).
		}
		this.dataProducers.clear();

		// Close every DataConsumer.
		for (const dataConsumer of this.dataConsumers.values())
		{
			dataConsumer.transportClosed();
		}
		this.dataConsumers.clear();

		this.safeEmit('routerclose');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Listen server was closed (this just happens in WebRtcTransports when their
	 * associated WebRtcServer is closed).
	 *
	 * @private
	 */
	listenServerClosed(): void
	{
		if (this.#closed)
		{
			return;
		}

		logger.debug('listenServerClosed()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.channel.removeAllListeners(this.internal.transportId);

		// Close every Producer.
		for (const producer of this.#producers.values())
		{
			producer.transportClosed();

			// NOTE: No need to tell the Router since it already knows (it has
			// been closed in fact).
		}
		this.#producers.clear();

		// Close every Consumer.
		for (const consumer of this.consumers.values())
		{
			consumer.transportClosed();
		}
		this.consumers.clear();

		// Close every DataProducer.
		for (const dataProducer of this.dataProducers.values())
		{
			dataProducer.transportClosed();

			// NOTE: No need to tell the Router since it already knows (it has
			// been closed in fact).
		}
		this.dataProducers.clear();

		// Close every DataConsumer.
		for (const dataConsumer of this.dataConsumers.values())
		{
			dataConsumer.transportClosed();
		}
		this.dataConsumers.clear();

		// Need to emit this event to let the parent Router know since
		// transport.listenServerClosed() is called by the listen server.
		// NOTE: Currently there is just WebRtcServer for WebRtcTransports.
		this.emit('@listenserverclose');

		this.safeEmit('listenserverclose');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Dump Transport.
	 *
	 * @abstract
	 */
	async dump(): Promise<any>
	{
		// Should not happen.
		throw new Error('method not implemented in the subclass');
	}

	/**
	 * Get Transport stats.
	 *
	 * @abstract
	 */
	async getStats(): Promise<any[]>
	{
		// Should not happen.
		throw new Error('method not implemented in the subclass');
	}

	/**
	 * Provide the Transport remote parameters.
	 *
	 * @abstract
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	async connect(params: any): Promise<void>
	{
		// Should not happen.
		throw new Error('method not implemented in the subclass');
	}

	/**
	 * Set maximum incoming bitrate for receiving media.
	 */
	async setMaxIncomingBitrate(bitrate: number): Promise<void>
	{
		logger.debug('setMaxIncomingBitrate() [bitrate:%s]', bitrate);

		/* Build Request. */

		const requestOffset = new FbsTransport.SetMaxIncomingBitrateRequestT(
			bitrate
		).pack(this.channel.bufferBuilder);

		await this.channel.request(
			FbsRequest.Method.TRANSPORT_SET_MAX_INCOMING_BITRATE,
			FbsRequest.Body.FBS_Transport_SetMaxIncomingBitrateRequest,
			requestOffset,
			this.internal.transportId
		);
	}

	/**
	 * Set maximum outgoing bitrate for sending media.
	 */
	async setMaxOutgoingBitrate(bitrate: number): Promise<void>
	{
		logger.debug('setMaxOutgoingBitrate() [bitrate:%s]', bitrate);

		/* Build Request. */

		const requestOffset = new FbsTransport.SetMaxOutgoingBitrateRequestT(
			bitrate
		).pack(this.channel.bufferBuilder);

		await this.channel.request(
			FbsRequest.Method.TRANSPORT_SET_MAX_OUTGOING_BITRATE,
			FbsRequest.Body.FBS_Transport_SetMaxOutgoingBitrateRequest,
			requestOffset,
			this.internal.transportId
		);
	}

	/**
	 * Create a Producer.
	 */
	async produce(
		{
			id = undefined,
			kind,
			rtpParameters,
			paused = false,
			keyFrameRequestDelay,
			appData
		}: ProducerOptions
	): Promise<Producer>
	{
		logger.debug('produce()');

		if (id && this.#producers.has(id))
		{
			throw new TypeError(`a Producer with same id "${id}" already exists`);
		}
		else if (![ 'audio', 'video' ].includes(kind))
		{
			throw new TypeError(`invalid kind "${kind}"`);
		}
		else if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		// This may throw.
		ortc.validateRtpParameters(rtpParameters);

		// If missing or empty encodings, add one.
		if (
			!rtpParameters.encodings ||
			!Array.isArray(rtpParameters.encodings) ||
			rtpParameters.encodings.length === 0
		)
		{
			rtpParameters.encodings = [ {} ];
		}

		// Don't do this in PipeTransports since there we must keep CNAME value in
		// each Producer.
		if (this.constructor.name !== 'PipeTransport')
		{
			// If CNAME is given and we don't have yet a CNAME for Producers in this
			// Transport, take it.
			if (!this.#cnameForProducers && rtpParameters.rtcp && rtpParameters.rtcp.cname)
			{
				this.#cnameForProducers = rtpParameters.rtcp.cname;
			}
			// Otherwise if we don't have yet a CNAME for Producers and the RTP parameters
			// do not include CNAME, create a random one.
			else if (!this.#cnameForProducers)
			{
				this.#cnameForProducers = uuidv4().substr(0, 8);
			}

			// Override Producer's CNAME.
			rtpParameters.rtcp = rtpParameters.rtcp || {};
			rtpParameters.rtcp.cname = this.#cnameForProducers;
		}

		const routerRtpCapabilities = this.#getRouterRtpCapabilities();

		// This may throw.
		const rtpMapping = ortc.getProducerRtpParametersMapping(
			rtpParameters, routerRtpCapabilities);

		// This may throw.
		const consumableRtpParameters = ortc.getConsumableRtpParameters(
			kind, rtpParameters, routerRtpCapabilities, rtpMapping);

		const producerId = id || uuidv4();
		const requestOffset = createProduceRequest({
			builder : this.channel.bufferBuilder,
			producerId,
			kind,
			rtpParameters,
			rtpMapping,
			keyFrameRequestDelay,
			paused
		});

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_PRODUCE,
			FbsRequest.Body.FBS_Transport_ProduceRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode the response. */
		const produceResponse = new FbsTransport.ProduceResponse();

		response.body(produceResponse);

		const status = produceResponse.unpack();

		const data =
		{
			kind,
			rtpParameters,
			type : utils.getProducerType(status.type),
			consumableRtpParameters
		};

		const producer = new Producer(
			{
				internal :
				{
					...this.internal,
					producerId
				},
				data,
				channel        : this.channel,
				appData,
				paused
			});

		this.#producers.set(producer.id, producer);
		producer.on('@close', () =>
		{
			this.#producers.delete(producer.id);
			this.emit('@producerclose', producer);
		});

		this.emit('@newproducer', producer);

		// Emit observer event.
		this.#observer.safeEmit('newproducer', producer);

		return producer;
	}

	/**
	 * Create a Consumer.
	 *
	 * @virtual
	 */
	async consume(
		{
			producerId,
			rtpCapabilities,
			paused = false,
			mid,
			preferredLayers,
			ignoreDtx = false,
			pipe = false,
			appData
		}: ConsumerOptions
	): Promise<Consumer>
	{
		logger.debug('consume()');

		if (!producerId || typeof producerId !== 'string')
		{
			throw new TypeError('missing producerId');
		}
		else if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}
		else if (mid && (typeof mid !== 'string' || mid.length === 0))
		{
			throw new TypeError('if given, mid must be non empty string');
		}

		// This may throw.
		ortc.validateRtpCapabilities(rtpCapabilities!);

		const producer = this.getProducerById(producerId);

		if (!producer)
		{
			throw Error(`Producer with id "${producerId}" not found`);
		}

		// This may throw.
		const rtpParameters = ortc.getConsumerRtpParameters(
			producer.consumableRtpParameters, rtpCapabilities!, pipe);

		// Set MID.
		if (!pipe)
		{
			if (mid)
			{
				rtpParameters.mid = mid;
			}
			else
			{
				rtpParameters.mid = `${this.#nextMidForConsumers++}`;

				// We use up to 8 bytes for MID (string).
				if (this.#nextMidForConsumers === 100000000)
				{
					logger.error(
						`consume() | reaching max MID value "${this.#nextMidForConsumers}"`);

					this.#nextMidForConsumers = 0;
				}
			}
		}

		const consumerId = uuidv4();
		const consumeRequestOffset = createConsumeRequest({
			builder : this.channel.bufferBuilder,
			producer,
			consumerId,
			rtpParameters,
			paused,
			preferredLayers,
			ignoreDtx,
			pipe
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
			type : pipe ? 'pipe' : producer.type as ConsumerType
		};

		const consumer = new Consumer(
			{
				internal :
				{
					...this.internal,
					consumerId
				},
				data,
				channel         : this.channel,
				appData,
				paused          : status.paused,
				producerPaused  : status.producerPaused,
				score           : status.score ?? undefined,
				preferredLayers : status.preferredLayers ?
					{
						spatialLayer  : status.preferredLayers.spatialLayer,
						temporalLayer : status.preferredLayers.temporalLayer !== null ?
							status.preferredLayers.temporalLayer :
							undefined
					} :
					undefined
			});

		this.consumers.set(consumer.id, consumer);
		consumer.on('@close', () => this.consumers.delete(consumer.id));
		consumer.on('@producerclose', () => this.consumers.delete(consumer.id));

		// Emit observer event.
		this.#observer.safeEmit('newconsumer', consumer);

		return consumer;
	}

	/**
	 * Create a DataProducer.
	 */
	async produceData(
		{
			id = undefined,
			sctpStreamParameters,
			label = '',
			protocol = '',
			appData
		}: DataProducerOptions = {}
	): Promise<DataProducer>
	{
		logger.debug('produceData()');

		if (id && this.dataProducers.has(id))
		{
			throw new TypeError(`a DataProducer with same id "${id}" already exists`);
		}
		else if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		let type: DataProducerType;

		// If this is not a DirectTransport, sctpStreamParameters are required.
		if (this.constructor.name !== 'DirectTransport')
		{
			type = 'sctp';

			// This may throw.
			ortc.validateSctpStreamParameters(sctpStreamParameters!);
		}
		// If this is a DirectTransport, sctpStreamParameters must not be given.
		else
		{
			type = 'direct';

			if (sctpStreamParameters)
			{
				logger.warn(
					'produceData() | sctpStreamParameters are ignored when producing data on a DirectTransport');
			}
		}

		const dataProducerId = id || uuidv4();
		const requestOffset = createProduceDataRequest({
			builder : this.channel.bufferBuilder,
			dataProducerId,
			type,
			sctpStreamParameters,
			label,
			protocol
		});

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_PRODUCE_DATA,
			FbsRequest.Body.FBS_Transport_ProduceDataRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode the response. */
		const produceResponse = new FbsDataProducer.DumpResponse();

		response.body(produceResponse);

		const data = parseDataProducerDump(produceResponse);
		const dataProducer = new DataProducer(
			{
				internal :
				{
					...this.internal,
					dataProducerId
				},
				data,
				channel        : this.channel,
				appData
			});

		this.dataProducers.set(dataProducer.id, dataProducer);
		dataProducer.on('@close', () =>
		{
			this.dataProducers.delete(dataProducer.id);
			this.emit('@dataproducerclose', dataProducer);
		});

		this.emit('@newdataproducer', dataProducer);

		// Emit observer event.
		this.#observer.safeEmit('newdataproducer', dataProducer);

		return dataProducer;
	}

	/**
	 * Create a DataConsumer.
	 */
	async consumeData(
		{
			dataProducerId,
			ordered,
			maxPacketLifeTime,
			maxRetransmits,
			appData
		}: DataConsumerOptions
	): Promise<DataConsumer>
	{
		logger.debug('consumeData()');

		if (!dataProducerId || typeof dataProducerId !== 'string')
		{
			throw new TypeError('missing dataProducerId');
		}
		else if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		const dataProducer = this.getDataProducerById(dataProducerId);

		if (!dataProducer)
		{
			throw Error(`DataProducer with id "${dataProducerId}" not found`);
		}

		let type: DataConsumerType;
		let sctpStreamParameters: SctpStreamParameters | undefined;
		let sctpStreamId: number;

		// If this is not a DirectTransport, use sctpStreamParameters from the
		// DataProducer (if type 'sctp') unless they are given in method parameters.
		if (this.constructor.name !== 'DirectTransport')
		{
			type = 'sctp';
			sctpStreamParameters =
				utils.clone(dataProducer.sctpStreamParameters) as SctpStreamParameters;

			// Override if given.
			if (ordered !== undefined)
			{
				sctpStreamParameters.ordered = ordered;
			}

			if (maxPacketLifeTime !== undefined)
			{
				sctpStreamParameters.maxPacketLifeTime = maxPacketLifeTime;
			}

			if (maxRetransmits !== undefined)
			{
				sctpStreamParameters.maxRetransmits = maxRetransmits;
			}

			// This may throw.
			sctpStreamId = this.getNextSctpStreamId();

			this.#sctpStreamIds![sctpStreamId] = 1;
			sctpStreamParameters.streamId = sctpStreamId;
		}
		// If this is a DirectTransport, sctpStreamParameters must not be used.
		else
		{
			type = 'direct';

			if (
				ordered !== undefined ||
				maxPacketLifeTime !== undefined ||
				maxRetransmits !== undefined
			)
			{
				logger.warn(
					'consumeData() | ordered, maxPacketLifeTime and maxRetransmits are ignored when consuming data on a DirectTransport');
			}
		}

		const { label, protocol } = dataProducer;
		const dataConsumerId = uuidv4();

		const requestOffset = createConsumeDataRequest({
			builder : this.channel.bufferBuilder,
			dataConsumerId,
			dataProducerId,
			type,
			sctpStreamParameters,
			label,
			protocol
		});

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_CONSUME_DATA,
			FbsRequest.Body.FBS_Transport_ConsumeDataRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode the response. */
		const consumeResponse = new FbsDataConsumer.DumpResponse();

		response.body(consumeResponse);

		const data = parseDataConsumerDump(consumeResponse);
		const dataConsumer = new DataConsumer(
			{
				internal :
				{
					...this.internal,
					dataConsumerId
				},
				data,
				channel        : this.channel,
				appData
			});

		this.dataConsumers.set(dataConsumer.id, dataConsumer);
		dataConsumer.on('@close', () =>
		{
			this.dataConsumers.delete(dataConsumer.id);

			if (this.#sctpStreamIds)
			{
				this.#sctpStreamIds[sctpStreamId] = 0;
			}
		});
		dataConsumer.on('@dataproducerclose', () =>
		{
			this.dataConsumers.delete(dataConsumer.id);

			if (this.#sctpStreamIds)
			{
				this.#sctpStreamIds[sctpStreamId] = 0;
			}
		});

		// Emit observer event.
		this.#observer.safeEmit('newdataconsumer', dataConsumer);

		return dataConsumer;
	}

	/**
	 * Enable 'trace' event.
	 */
	async enableTraceEvent(types: TransportTraceEventType[] = []): Promise<void>
	{
		logger.debug('enableTraceEvent()');

		if (!Array.isArray(types))
			throw new TypeError('types must be an array');
		if (types.find((type) => typeof type !== 'string'))
			throw new TypeError('every type must be a string');

		/* Build Request. */

		const requestOffset = new FbsTransport.EnableTraceEventRequestT(
			types
		).pack(this.channel.bufferBuilder);

		await this.channel.request(
			FbsRequest.Method.TRANSPORT_ENABLE_TRACE_EVENT,
			FbsRequest.Body.FBS_Transport_EnableTraceEventRequest,
			requestOffset,
			this.internal.transportId
		);
	}

	private getNextSctpStreamId(): number
	{
		if (
			!this.#data.sctpParameters ||
			typeof this.#data.sctpParameters.MIS !== 'number'
		)
		{
			throw new TypeError('missing sctpParameters.MIS');
		}

		const numStreams = this.#data.sctpParameters.MIS;

		if (!this.#sctpStreamIds)
		{
			this.#sctpStreamIds = Buffer.alloc(numStreams, 0);
		}

		let sctpStreamId;

		for (let idx = 0; idx < this.#sctpStreamIds.length; ++idx)
		{
			sctpStreamId = (this.#nextSctpStreamId + idx) % this.#sctpStreamIds.length;

			if (!this.#sctpStreamIds[sctpStreamId])
			{
				this.#nextSctpStreamId = sctpStreamId + 1;

				return sctpStreamId;
			}
		}

		throw new Error('no sctpStreamId available');
	}
}

export function fbsSctpState2StcpState(fbsSctpState: FbsSctpState): SctpState
{
	switch (fbsSctpState)
	{
		case FbsSctpState.NEW:
			return 'new';
		case FbsSctpState.CONNECTING:
			return 'connecting';
		case FbsSctpState.CONNECTED:
			return 'connected';
		case FbsSctpState.FAILED:
			return 'failed';
		case FbsSctpState.CLOSED:
			return 'closed';
		default:
			throw new TypeError(`invalid SctpState: ${fbsSctpState}`);
	}
}

export function parseTuple(binary: FbsTransport.Tuple): TransportTuple
{
	return {
		localIp    : binary.localIp()!,
		localPort  : binary.localPort(),
		remoteIp   : binary.remoteIp() ?? undefined,
		remotePort : binary.remotePort(),
		protocol   : binary.protocol()! as TransportProtocol
	};
}

export function parseBaseTransportDump(
	binary: FbsTransport.Dump
): BaseTransportDump
{
	// Retrieve producerIds.
	const producerIds = utils.parseVector<string>(binary, 'producerIds');
	// Retrieve consumerIds.
	const consumerIds = utils.parseVector<string>(binary, 'consumerIds');
	// Retrieve map SSRC consumerId.
	const mapSsrcConsumerId = utils.parseUint32StringVector(binary, 'mapSsrcConsumerId');
	// Retrieve map RTX SSRC consumerId.
	const mapRtxSsrcConsumerId = utils.parseUint32StringVector(binary, 'mapRtxSsrcConsumerId');
	// Retrieve dataProducerIds.
	const dataProducerIds = utils.parseVector<string>(binary, 'dataProducerIds');
	// Retrieve dataConsumerIds.
	const dataConsumerIds = utils.parseVector<string>(binary, 'dataConsumerIds');
	// Retrieve recvRtpHeaderExtesions.
	const recvRtpHeaderExtensions = utils.parseStringUint8Vector(binary, 'recvRtpHeaderExtensions');
	// Retrieve RtpListener.
	const rtpListener = parseRtpListenerDump(binary.rtpListener()!);

	// Retrieve SctpParameters.
	const fbsSctpParameters = binary.sctpParameters();
	let sctpParameters: SctpParameters | undefined;

	if (fbsSctpParameters)
	{
		sctpParameters = parseSctpParametersDump(fbsSctpParameters);
	}

	// Retrieve sctpState.
	const sctpState = binary.sctpState() === '' ? undefined : binary.sctpState() as SctpState;

	// Retrive sctpListener.
	const sctpListener = binary.sctpListener() ?
		parseSctpListenerDump(binary.sctpListener()!) :
		undefined;

	// Retrieve traceEventTypes.
	const traceEventTypes = utils.parseVector<string>(binary, 'traceEventTypes');

	return {
		id                      : binary.id()!,
		direct                  : binary.direct(),
		producerIds             : producerIds,
		consumerIds             : consumerIds,
		mapSsrcConsumerId       : mapSsrcConsumerId,
		mapRtxSsrcConsumerId    : mapRtxSsrcConsumerId,
		dataProducerIds         : dataProducerIds,
		dataConsumerIds         : dataConsumerIds,
		recvRtpHeaderExtensions : recvRtpHeaderExtensions,
		rtpListener             : rtpListener,
		maxMessageSize          : binary.maxMessageSize(),
		sctpParameters          : sctpParameters,
		sctpState               : sctpState,
		sctpListener            : sctpListener,
		traceEventTypes         : traceEventTypes
	};
}

export function parseBaseTransportStats(
	binary: FbsTransport.Stats
): BaseTransportStats
{
	const sctpState = binary.sctpState() === '' ? undefined : binary.sctpState() as SctpState;

	return {
		transportId              : binary.transportId()!,
		timestamp                : Number(binary.timestamp()),
		sctpState,
		bytesReceived            : Number(binary.bytesReceived()),
		recvBitrate              : Number(binary.recvBitrate()),
		bytesSent                : Number(binary.bytesSent()),
		sendBitrate              : Number(binary.sendBitrate()),
		rtpBytesReceived         : Number(binary.rtpBytesReceived()),
		rtpRecvBitrate           : Number(binary.rtpRecvBitrate()),
		rtpBytesSent             : Number(binary.rtpBytesSent()),
		rtpSendBitrate           : Number(binary.rtpSendBitrate()),
		rtxBytesReceived         : Number(binary.rtxBytesReceived()),
		rtxRecvBitrate           : Number(binary.rtxRecvBitrate()),
		rtxBytesSent             : Number(binary.rtxBytesSent()),
		rtxSendBitrate           : Number(binary.rtxSendBitrate()),
		probationBytesSent       : Number(binary.probationBytesSent()),
		probationSendBitrate     : Number(binary.probationSendBitrate()),
		availableOutgoingBitrate : Number(binary.availableOutgoingBitrate()),
		availableIncomingBitrate : Number(binary.availableIncomingBitrate()),
		maxIncomingBitrate       : binary.maxIncomingBitrate() ?
			Number(binary.maxIncomingBitrate()) :
			undefined
	};
}

export function parseTransportTraceEventData(
	trace: FbsTransport.TraceNotification
): TransportTraceEventData
{
	switch (trace.type())
	{
		case FbsTransport.TraceType.BWE:
		{
			const info = new FbsTransport.BweTraceInfo();

			trace.info(info);

			return {
				type      : 'bwe',
				timestamp : Number(trace.timestamp()),
				direction : trace.direction() === FbsTransport.TraceDirection.DIRECTION_IN ? 'in' : 'out',
				info      : parseBweTraceInfo(info!)
			};
		}

		case FbsTransport.TraceType.PROBATION:
		{
			return {
				type      : 'probation',
				timestamp : Number(trace.timestamp()),
				direction : trace.direction() === FbsTransport.TraceDirection.DIRECTION_IN ? 'in' : 'out',
				info      : {}
			};
		}
	}
}

function parseBweTraceInfo(binary: FbsTransport.BweTraceInfo):
{
  desiredBitrate:number;
  effectiveDesiredBitrate:number;
  minBitrate:number;
  maxBitrate:number;
  startBitrate:number;
  maxPaddingBitrate:number;
  availableBitrate:number;
  bweType:'transport-cc' | 'remb';
}
{
	return {
		desiredBitrate          : binary.desiredBitrate(),
		effectiveDesiredBitrate : binary.effectiveDesiredBitrate(),
		minBitrate              : binary.minBitrate(),
		maxBitrate              : binary.maxBitrate(),
		startBitrate            : binary.startBitrate(),
		maxPaddingBitrate       : binary.maxPaddingBitrate(),
		availableBitrate        : binary.availableBitrate(),
		bweType                 : binary.bweType() === FbsTransport.BweType.TRANSPORT_CC ?
			'transport-cc' :
			'remb'
	};
}

function createConsumeRequest({
	builder,
	producer,
	consumerId,
	rtpParameters,
	paused,
	preferredLayers,
	ignoreDtx,
	pipe
} : {
	builder: flatbuffers.Builder;
	producer: Producer;
	consumerId: string;
	rtpParameters: RtpParameters;
	paused: boolean;
	preferredLayers?: ConsumerLayers;
	ignoreDtx?: boolean;
	pipe: boolean;
}): number
{
	const rtpParametersOffset = serializeRtpParameters(builder, rtpParameters);
	const consumerIdOffset = builder.createString(consumerId);
	const producerIdOffset = builder.createString(producer.id);
	let consumableRtpEncodingsOffset: number | undefined;
	let preferredLayersOffset: number | undefined;

	if (producer.consumableRtpParameters.encodings)
	{
		consumableRtpEncodingsOffset = serializeRtpEncodingParameters(
			builder, producer.consumableRtpParameters.encodings
		);
	}

	if (preferredLayers)
	{
		FbsConsumer.ConsumerLayers.startConsumerLayers(builder);
		FbsConsumer.ConsumerLayers.addSpatialLayer(builder, preferredLayers.spatialLayer);

		if (preferredLayers.temporalLayer !== undefined)
		{
			FbsConsumer.ConsumerLayers.addTemporalLayer(
				builder, preferredLayers.temporalLayer
			);
		}

		preferredLayersOffset = FbsConsumer.ConsumerLayers.endConsumerLayers(builder);
	}

	const ConsumeRequest = FbsRequest.ConsumeRequest;

	// Create Consume Request.
	ConsumeRequest.startConsumeRequest(builder);
	ConsumeRequest.addConsumerId(builder, consumerIdOffset);
	ConsumeRequest.addProducerId(builder, producerIdOffset);
	ConsumeRequest.addKind(
		builder, producer.kind === 'audio' ? FbsMediaKind.AUDIO : FbsMediaKind.VIDEO);
	ConsumeRequest.addRtpParameters(builder, rtpParametersOffset);
	ConsumeRequest.addType(
		builder,
		utils.getRtpParametersType(producer.type, pipe)
	);

	if (consumableRtpEncodingsOffset)
		ConsumeRequest.addConsumableRtpEncodings(builder, consumableRtpEncodingsOffset);

	ConsumeRequest.addPaused(builder, paused);

	if (preferredLayersOffset)
		ConsumeRequest.addPreferredLayers(builder, preferredLayersOffset);

	ConsumeRequest.addIgnoreDtx(builder, Boolean(ignoreDtx));

	return ConsumeRequest.endConsumeRequest(builder);
}

function createProduceRequest({
	builder,
	producerId,
	kind,
	rtpParameters,
	rtpMapping,
	keyFrameRequestDelay,
	paused
} : {
	builder : flatbuffers.Builder;
	producerId: string;
	kind: MediaKind;
	rtpParameters: RtpParameters;
	rtpMapping: ortc.RtpMapping;
	keyFrameRequestDelay?: number;
	paused: boolean;
}): number
{
	const producerIdOffset = builder.createString(producerId);
	const rtpParametersOffset = serializeRtpParameters(builder, rtpParameters);
	const rtpMappingOffset = ortc.serializeRtpMapping(builder, rtpMapping);

	FbsTransport.ProduceRequest.startProduceRequest(builder);
	FbsTransport.ProduceRequest.addProducerId(builder, producerIdOffset);
	FbsTransport.ProduceRequest.addKind(
		builder, kind === 'audio' ? FbsMediaKind.AUDIO : FbsMediaKind.VIDEO);
	FbsTransport.ProduceRequest.addRtpParameters(builder, rtpParametersOffset);
	FbsTransport.ProduceRequest.addRtpMapping(builder, rtpMappingOffset);
	FbsTransport.ProduceRequest.addKeyFrameRequestDelay(builder, keyFrameRequestDelay ?? 0);
	FbsTransport.ProduceRequest.addPaused(builder, paused);

	return FbsTransport.ProduceRequest.endProduceRequest(builder);
}

function createProduceDataRequest({
	builder,
	dataProducerId,
	type,
	sctpStreamParameters,
	label,
	protocol
} : {
	builder : flatbuffers.Builder;
	dataProducerId: string;
	type: DataProducerType;
	sctpStreamParameters?: SctpStreamParameters;
	label: string;
	protocol: string;
}): number
{
	const dataProducerIdOffset = builder.createString(dataProducerId);
	const typeOffset = builder.createString(type);
	const labelOffset = builder.createString(label);
	const protocolOffset = builder.createString(protocol);

	let sctpStreamParametersOffset = 0;

	if (sctpStreamParameters)
	{
		sctpStreamParametersOffset = serializeSctpStreamParameters(
			builder, sctpStreamParameters
		);
	}

	FbsTransport.ProduceDataRequest.startProduceDataRequest(builder);
	FbsTransport.ProduceDataRequest.addDataProducerId(builder, dataProducerIdOffset);
	FbsTransport.ProduceDataRequest.addType(builder, typeOffset);

	if (sctpStreamParametersOffset)
	{
		FbsTransport.ProduceDataRequest.addSctpStreamParameters(
			builder, sctpStreamParametersOffset
		);
	}

	FbsTransport.ProduceDataRequest.addLabel(builder, labelOffset);
	FbsTransport.ProduceDataRequest.addProtocol(builder, protocolOffset);

	return FbsTransport.ProduceDataRequest.endProduceDataRequest(builder);
}

function createConsumeDataRequest({
	builder,
	dataConsumerId,
	dataProducerId,
	type,
	sctpStreamParameters,
	label,
	protocol
} : {
	builder : flatbuffers.Builder;
	dataConsumerId: string;
	dataProducerId: string;
	type: DataConsumerType;
	sctpStreamParameters?: SctpStreamParameters;
	label: string;
	protocol: string;
}): number
{
	const dataConsumerIdOffset = builder.createString(dataConsumerId);
	const dataProducerIdOffset = builder.createString(dataProducerId);
	const typeOffset = builder.createString(type);
	const labelOffset = builder.createString(label);
	const protocolOffset = builder.createString(protocol);

	let sctpStreamParametersOffset = 0;

	if (sctpStreamParameters)
	{
		sctpStreamParametersOffset = serializeSctpStreamParameters(
			builder, sctpStreamParameters
		);
	}

	FbsTransport.ConsumeDataRequest.startConsumeDataRequest(builder);
	FbsTransport.ConsumeDataRequest.addDataConsumerId(builder, dataConsumerIdOffset);
	FbsTransport.ConsumeDataRequest.addDataProducerId(builder, dataProducerIdOffset);
	FbsTransport.ConsumeDataRequest.addType(builder, typeOffset);

	if (sctpStreamParametersOffset)
	{
		FbsTransport.ConsumeDataRequest.addSctpStreamParameters(
			builder, sctpStreamParametersOffset
		);
	}

	FbsTransport.ConsumeDataRequest.addLabel(builder, labelOffset);
	FbsTransport.ConsumeDataRequest.addProtocol(builder, protocolOffset);

	return FbsTransport.ConsumeDataRequest.endConsumeDataRequest(builder);
}

function parseRtpListenerDump(binary: FbsTransport.RtpListener): RtpListenerDump
{
	// Retrieve ssrcTable.
	const ssrcTable = utils.parseUint32StringVector(binary, 'ssrcTable');
	// Retrieve midTable.
	const midTable = utils.parseUint32StringVector(binary, 'midTable');
	// Retrieve ridTable.
	const ridTable = utils.parseUint32StringVector(binary, 'ridTable');

	return {
		ssrcTable,
		midTable,
		ridTable
	};
}

function parseSctpListenerDump(binary: FbsTransport.SctpListener): SctpListenerDump
{
	// Retrieve streamIdTable.
	const streamIdTable = utils.parseUint32StringVector(binary, 'streamIdTable');

	return {
		streamIdTable
	};
}
