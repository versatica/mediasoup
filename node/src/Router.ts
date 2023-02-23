import { v4 as uuidv4 } from 'uuid';
import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import * as ortc from './ortc';
import { InvalidStateError } from './errors';
import { Channel } from './Channel';
import { Transport, TransportListenIp } from './Transport';
import { WebRtcTransport, WebRtcTransportOptions, parseWebRtcTransportDumpResponse } from './WebRtcTransport';
import { PlainTransport, PlainTransportOptions, parsePlainTransportDumpResponse } from './PlainTransport';
import { PipeTransport, PipeTransportOptions, parsePipeTransportDumpResponse } from './PipeTransport';
import { DirectTransport, DirectTransportOptions, parseDirectTransportDumpResponse } from './DirectTransport';
import { Producer } from './Producer';
import { Consumer } from './Consumer';
import { DataProducer } from './DataProducer';
import { DataConsumer } from './DataConsumer';
import { RtpObserver } from './RtpObserver';
import { ActiveSpeakerObserver, ActiveSpeakerObserverOptions } from './ActiveSpeakerObserver';
import { AudioLevelObserver, AudioLevelObserverOptions } from './AudioLevelObserver';
import { RtpCapabilities, RtpCodecCapability } from './RtpParameters';
import { NumSctpStreams } from './SctpParameters';
import * as FbsActiveSpeakerObserver from './fbs/activeSpeakerObserver_generated';
import * as FbsAudioLevelObserver from './fbs/audioLevelObserver_generated';
import * as FbsRequest from './fbs/request_generated';
import * as FbsRouter from './fbs/router_generated';
import * as FbsPlainTransport from './fbs/plainTransport_generated';
import * as FbsPipeTransport from './fbs/pipeTransport_generated';
import * as FbsDirectTransport from './fbs/directTransport_generated';
import * as FbsWebRtcTransport from './fbs/webRtcTransport_generated';
import * as FbsTransport from './fbs/transport_generated';
import * as FbsSctpParameters from './fbs/sctpParameters_generated';

export type RouterOptions =
{
	/**
	 * Router media codecs.
	 */
	mediaCodecs?: RtpCodecCapability[];

	/**
	 * Custom application data.
	 */
	appData?: Record<string, unknown>;
};

export type PipeToRouterOptions =
{
	/**
	 * The id of the Producer to consume.
	 */
	producerId?: string;

	/**
	 * The id of the DataProducer to consume.
	 */
	dataProducerId?: string;

	/**
	 * Target Router instance.
	 */
	router: Router;

	/**
	 * IP used in the PipeTransport pair. Default '127.0.0.1'.
	 */
	listenIp?: TransportListenIp | string;

	/**
	 * Create a SCTP association. Default true.
	 */
	enableSctp?: boolean;

	/**
	 * SCTP streams number.
	 */
	numSctpStreams?: NumSctpStreams;

	/**
	 * Enable RTX and NACK for RTP retransmission.
	 */
	enableRtx?: boolean;

	/**
	 * Enable SRTP.
	 */
	enableSrtp?: boolean;
};

export type PipeToRouterResult =
{
	/**
	 * The Consumer created in the current Router.
	 */
	pipeConsumer?: Consumer;

	/**
	 * The Producer created in the target Router.
	 */
	pipeProducer?: Producer;

	/**
	 * The DataConsumer created in the current Router.
	 */
	pipeDataConsumer?: DataConsumer;

	/**
	 * The DataProducer created in the target Router.
	 */
	pipeDataProducer?: DataProducer;
};

type PipeTransportPair =
{
	[key: string]: PipeTransport;
};

export type RouterEvents =
{ 
	workerclose: [];
	// Private events.
	'@close': [];
};

export type RouterObserverEvents =
{
	close: [];
	newtransport: [Transport];
	newrtpobserver: [RtpObserver];
};

export type RouterInternal =
{
	routerId: string;
};

type RouterData =
{
	rtpCapabilities: RtpCapabilities;
};

const logger = new Logger('Router');

export class Router extends EnhancedEventEmitter<RouterEvents>
{
	// Internal data.
	readonly #internal: RouterInternal;

	// Router data.
	readonly #data: RouterData;

	// Channel instance.
	readonly #channel: Channel;

	// Closed flag.
	#closed = false;

	// Custom app data.
	readonly #appData: Record<string, unknown>;

	// Transports map.
	readonly #transports: Map<string, Transport> = new Map();

	// Producers map.
	readonly #producers: Map<string, Producer> = new Map();

	// RtpObservers map.
	readonly #rtpObservers: Map<string, RtpObserver> = new Map();

	// DataProducers map.
	readonly #dataProducers: Map<string, DataProducer> = new Map();

	// Map of PipeTransport pair Promises indexed by the id of the Router in
	// which pipeToRouter() was called.
	readonly #mapRouterPairPipeTransportPairPromise:
		Map<string, Promise<PipeTransportPair>> = new Map();

	// Observer instance.
	readonly #observer = new EnhancedEventEmitter<RouterObserverEvents>();

	/**
	 * @private
	 */
	constructor(
		{
			internal,
			data,
			channel,
			appData
		}:
		{
			internal: RouterInternal;
			data: RouterData;
			channel: Channel;
			appData?: Record<string, unknown>;
		}
	)
	{
		super();

		logger.debug('constructor()');

		this.#internal = internal;
		this.#data = data;
		this.#channel = channel;
		this.#appData = appData || {};
	}

	/**
	 * Router id.
	 */
	get id(): string
	{
		return this.#internal.routerId;
	}

	/**
	 * Whether the Router is closed.
	 */
	get closed(): boolean
	{
		return this.#closed;
	}

	/**
	 * RTP capabilities of the Router.
	 */
	get rtpCapabilities(): RtpCapabilities
	{
		return this.#data.rtpCapabilities;
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
	get observer(): EnhancedEventEmitter<RouterObserverEvents>
	{
		return this.#observer;
	}

	/**
	 * @private
	 * Just for testing purposes.
	 */
	get transportsForTesting(): Map<string, Transport>
	{
		return this.#transports;
	}

	/**
	 * Close the Router.
	 */
	close(): void
	{
		if (this.#closed)
		{
			return;
		}

		logger.debug('close()');

		this.#closed = true;

		const requestOffset = new FbsRequest.CloseRouterRequestT(
			this.#internal.routerId).pack(this.#channel.bufferBuilder);

		this.#channel.request(
			FbsRequest.Method.WORKER_CLOSE_ROUTER,
			FbsRequest.Body.FBS_Worker_CloseRouterRequest,
			requestOffset)
			.catch(() => {});

		// Close every Transport.
		for (const transport of this.#transports.values())
		{
			transport.routerClosed();
		}
		this.#transports.clear();

		// Clear the Producers map.
		this.#producers.clear();

		// Close every RtpObserver.
		for (const rtpObserver of this.#rtpObservers.values())
		{
			rtpObserver.routerClosed();
		}
		this.#rtpObservers.clear();

		// Clear the DataProducers map.
		this.#dataProducers.clear();

		this.emit('@close');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Worker was closed.
	 *
	 * @private
	 */
	workerClosed(): void
	{
		if (this.#closed)
		{
			return;
		}

		logger.debug('workerClosed()');

		this.#closed = true;

		// Close every Transport.
		for (const transport of this.#transports.values())
		{
			transport.routerClosed();
		}
		this.#transports.clear();

		// Clear the Producers map.
		this.#producers.clear();

		// Close every RtpObserver.
		for (const rtpObserver of this.#rtpObservers.values())
		{
			rtpObserver.routerClosed();
		}
		this.#rtpObservers.clear();

		// Clear the DataProducers map.
		this.#dataProducers.clear();

		this.safeEmit('workerclose');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Dump Router.
	 */
	async dump(): Promise<any>
	{
		logger.debug('dump()');

		// Send the request and wait for the response.
		const response = await this.#channel.request(
			FbsRequest.Method.ROUTER_DUMP,
			undefined,
			undefined,
			this.#internal.routerId
		);

		/* Decode the response. */
		const dump = new FbsRouter.DumpResponse();

		response.body(dump);

		return dump.unpack();
	}

	/**
	 * Create a WebRtcTransport.
	 */
	async createWebRtcTransport(
		{
			webRtcServer,
			listenIps,
			port,
			enableUdp = true,
			enableTcp = false,
			preferUdp = false,
			preferTcp = false,
			initialAvailableOutgoingBitrate = 600000,
			enableSctp = false,
			numSctpStreams = { OS: 1024, MIS: 1024 },
			maxSctpMessageSize = 262144,
			sctpSendBufferSize = 262144,
			appData
		}: WebRtcTransportOptions
	): Promise<WebRtcTransport>
	{
		logger.debug('createWebRtcTransport()');

		if (!webRtcServer && !Array.isArray(listenIps))
		{
			throw new TypeError('missing webRtcServer and listenIps (one of them is mandatory)');
		}
		else if (
			numSctpStreams &&
			(typeof numSctpStreams.OS !== 'number' || typeof numSctpStreams.MIS !== 'number')
		)
		{
			throw new TypeError('if given, numSctpStreams must contain OS and MID');
		}
		else if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		if (listenIps)
		{
			if (listenIps.length === 0)
			{
				throw new TypeError('empty listenIps array provided');
			}

			listenIps = listenIps.map((listenIp) =>
			{
				if (typeof listenIp === 'string' && listenIp)
				{
					return { ip: listenIp };
				}
				else if (typeof listenIp === 'object')
				{
					return {
						ip          : listenIp.ip,
						announcedIp : listenIp.announcedIp || undefined
					};
				}
				else
				{
					throw new TypeError('wrong listenIp');
				}
			});
		}

		const transportId = uuidv4();

		/* Build Request. */

		let webRtcTransportListenServer:
			FbsWebRtcTransport.ListenServerT | undefined;
		let webRtcTransportListenIndividual:
			FbsWebRtcTransport.ListenIndividualT | undefined;

		if (webRtcServer)
		{
			webRtcTransportListenServer =
				new FbsWebRtcTransport.ListenServerT(webRtcServer.id);
		}
		else
		{
			const fbsListenIps: FbsTransport.ListenIpT[] = [];

			for (const listenIp of listenIps as any[])
			{
				fbsListenIps.push(
					new FbsTransport.ListenIpT(listenIp.ip, listenIp.announcedIp));
			}

			webRtcTransportListenIndividual =
				new FbsWebRtcTransport.ListenIndividualT(fbsListenIps, port);
		}

		const baseTransportOptions = new FbsTransport.OptionsT(
			undefined /* direct */,
			undefined /* maxMessageSize */,
			initialAvailableOutgoingBitrate,
			enableSctp,
			new FbsSctpParameters.NumSctpStreamsT(numSctpStreams.OS, numSctpStreams.MIS),
			maxSctpMessageSize,
			sctpSendBufferSize,
			true /* isDataChannel */
		);

		const webRtcTransportOptions = new FbsRouter.WebRtcTransportOptionsT(
			baseTransportOptions,
			webRtcServer ?
				FbsWebRtcTransport.Listen.ListenServer :
				FbsWebRtcTransport.Listen.ListenIndividual,
			webRtcServer ? webRtcTransportListenServer : webRtcTransportListenIndividual,
			enableUdp,
			enableTcp,
			preferUdp,
			preferTcp
		);

		const requestOffset = new FbsRouter.CreateWebRtcTransportRequestT(
			transportId, webRtcTransportOptions
		).pack(this.#channel.bufferBuilder);

		const response = await this.#channel.request(
			webRtcServer
				? FbsRequest.Method.ROUTER_CREATE_WEBRTC_TRANSPORT_WITH_SERVER
				: FbsRequest.Method.ROUTER_CREATE_WEBRTC_TRANSPORT,
			FbsRequest.Body.FBS_Router_CreateWebRtcTransportRequest,
			requestOffset,
			this.#internal.routerId
		);

		/* Decode the response. */

		const data = new FbsWebRtcTransport.DumpResponse();

		response.body(data);

		const webRtcTransportData = parseWebRtcTransportDumpResponse(data);

		const transport = new WebRtcTransport(
			{
				internal :
				{
					...this.#internal,
					transportId : transportId
				},
				data                     : webRtcTransportData,
				channel                  : this.#channel,
				appData,
				getRouterRtpCapabilities : (): RtpCapabilities => this.#data.rtpCapabilities,
				getProducerById          : (producerId: string): Producer | undefined => (
					this.#producers.get(producerId)
				),
				getDataProducerById : (dataProducerId: string): DataProducer | undefined => (
					this.#dataProducers.get(dataProducerId)
				)
			});

		this.#transports.set(transport.id, transport);
		transport.on('@close', () => this.#transports.delete(transport.id));
		transport.on('@listenserverclose', () => this.#transports.delete(transport.id));
		transport.on('@newproducer', (producer: Producer) => this.#producers.set(producer.id, producer));
		transport.on('@producerclose', (producer: Producer) => this.#producers.delete(producer.id));
		transport.on('@newdataproducer', (dataProducer: DataProducer) => (
			this.#dataProducers.set(dataProducer.id, dataProducer)
		));
		transport.on('@dataproducerclose', (dataProducer: DataProducer) => (
			this.#dataProducers.delete(dataProducer.id)
		));

		if (webRtcServer)
		{
			webRtcServer.handleWebRtcTransport(transport);
		}

		// Emit observer event.
		this.#observer.safeEmit('newtransport', transport);

		return transport;
	}

	/**
	 * Create a PlainTransport.
	 */
	async createPlainTransport(
		{
			listenIp,
			port,
			rtcpMux = true,
			comedia = false,
			enableSctp = false,
			numSctpStreams = { OS: 1024, MIS: 1024 },
			maxSctpMessageSize = 262144,
			sctpSendBufferSize = 262144,
			enableSrtp = false,
			srtpCryptoSuite = 'AES_CM_128_HMAC_SHA1_80',
			appData
		}: PlainTransportOptions
	): Promise<PlainTransport>
	{
		logger.debug('createPlainTransport()');

		if (!listenIp)
		{
			throw new TypeError('missing listenIp');
		}
		else if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		if (typeof listenIp === 'string' && listenIp)
		{
			listenIp = { ip: listenIp };
		}
		else if (typeof listenIp === 'object')
		{
			listenIp =
			{
				ip          : listenIp.ip,
				announcedIp : listenIp.announcedIp || undefined
			};
		}
		else
		{
			throw new TypeError('wrong listenIp');
		}

		const transportId = uuidv4();

		// NOTE: This check should not be needed since
		// FbsTransport.OptionsT should throw.
		// In absence of 'OS' or 'MIS' it does not throw in Linux.
		// TODO: Investigate the cause.
		if (!numSctpStreams.OS)
		{
			throw new TypeError('missing OS');
		}
		if (!numSctpStreams.MIS)
		{
			throw new TypeError('missing MIS');
		}

		/* Build Request. */

		const baseTransportOptions = new FbsTransport.OptionsT(
			undefined /* direct */,
			undefined /* maxMessageSize */,
			undefined /* initialAvailableOutgoingBitrate */,
			enableSctp,
			new FbsSctpParameters.NumSctpStreamsT(numSctpStreams.OS, numSctpStreams.MIS),
			maxSctpMessageSize,
			sctpSendBufferSize,
			false /* isDataChannel */
		);

		// NOTE: This check should not be needed since
		// FbsRouter.PlainTransportOptionsT should throw.
		// In absence of 'ip' it does not throw in Linux.
		// TODO: Investigate the cause.
		if (!listenIp.ip)
		{
			throw new TypeError('missing ip');
		}

		const plainTransportOptions = new FbsRouter.PlainTransportOptionsT(
			baseTransportOptions,
			new FbsTransport.ListenIpT(listenIp.ip, listenIp.announcedIp),
			port,
			rtcpMux,
			comedia,
			enableSrtp,
			srtpCryptoSuite
		);

		const requestOffset = new FbsRouter.CreatePlainTransportRequestT(
			transportId, plainTransportOptions
		).pack(this.#channel.bufferBuilder);

		const response = await this.#channel.request(
			FbsRequest.Method.ROUTER_CREATE_PLAIN_TRANSPORT,
			FbsRequest.Body.FBS_Router_CreatePlainTransportRequest,
			requestOffset,
			this.#internal.routerId
		);

		/* Decode the response. */

		const data = new FbsPlainTransport.DumpResponse();

		response.body(data);

		const plainTransportData = parsePlainTransportDumpResponse(data);

		const transport = new PlainTransport(
			{
				internal :
				{
					...this.#internal,
					transportId : transportId
				},
				data                     : plainTransportData,
				channel                  : this.#channel,
				appData,
				getRouterRtpCapabilities : (): RtpCapabilities => this.#data.rtpCapabilities,
				getProducerById          : (producerId: string): Producer | undefined => (
					this.#producers.get(producerId)
				),
				getDataProducerById : (dataProducerId: string): DataProducer | undefined => (
					this.#dataProducers.get(dataProducerId)
				)
			});

		this.#transports.set(transport.id, transport);
		transport.on('@close', () => this.#transports.delete(transport.id));
		transport.on('@listenserverclose', () => this.#transports.delete(transport.id));
		transport.on('@newproducer', (producer: Producer) => this.#producers.set(producer.id, producer));
		transport.on('@producerclose', (producer: Producer) => this.#producers.delete(producer.id));
		transport.on('@newdataproducer', (dataProducer: DataProducer) => (
			this.#dataProducers.set(dataProducer.id, dataProducer)
		));
		transport.on('@dataproducerclose', (dataProducer: DataProducer) => (
			this.#dataProducers.delete(dataProducer.id)
		));

		// Emit observer event.
		this.#observer.safeEmit('newtransport', transport);

		return transport;
	}

	/**
	 * Create a PipeTransport.
	 */
	async createPipeTransport(
		{
			listenIp,
			port,
			enableSctp = false,
			numSctpStreams = { OS: 1024, MIS: 1024 },
			maxSctpMessageSize = 268435456,
			sctpSendBufferSize = 268435456,
			enableRtx = false,
			enableSrtp = false,
			appData
		}: PipeTransportOptions
	): Promise<PipeTransport>
	{
		logger.debug('createPipeTransport()');

		if (!listenIp)
		{
			throw new TypeError('missing listenIp');
		}
		else if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		if (typeof listenIp === 'string' && listenIp)
		{
			listenIp = { ip: listenIp };
		}
		else if (typeof listenIp === 'object')
		{
			listenIp =
			{
				ip          : listenIp.ip,
				announcedIp : listenIp.announcedIp || undefined
			};
		}
		else
		{
			throw new TypeError('wrong listenIp');
		}

		const transportId = uuidv4();

		/* Build Request. */

		const baseTransportOptions = new FbsTransport.OptionsT(
			undefined /* direct */,
			undefined /* maxMessageSize */,
			undefined /* initialAvailableOutgoingBitrate */,
			enableSctp,
			new FbsSctpParameters.NumSctpStreamsT(numSctpStreams.OS, numSctpStreams.MIS),
			maxSctpMessageSize,
			sctpSendBufferSize,
			false /* isDataChannel */
		);

		const pipeTransportOptions = new FbsRouter.PipeTransportOptionsT(
			baseTransportOptions,
			new FbsTransport.ListenIpT(listenIp.ip, listenIp.announcedIp),
			port,
			enableRtx,
			enableSrtp
		);

		const requestOffset = new FbsRouter.CreatePipeTransportRequestT(
			transportId, pipeTransportOptions
		).pack(this.#channel.bufferBuilder);

		const response = await this.#channel.request(
			FbsRequest.Method.ROUTER_CREATE_PIPE_TRANSPORT,
			FbsRequest.Body.FBS_Router_CreatePipeTransportRequest,
			requestOffset,
			this.#internal.routerId
		);

		/* Decode the response. */

		const data = new FbsPipeTransport.DumpResponse();

		response.body(data);

		const plainTransportData = parsePipeTransportDumpResponse(data);

		const transport = new PipeTransport(
			{
				internal :
				{
					...this.#internal,
					transportId
				},
				data                     : plainTransportData,
				channel                  : this.#channel,
				appData,
				getRouterRtpCapabilities : (): RtpCapabilities => this.#data.rtpCapabilities,
				getProducerById          : (producerId: string): Producer | undefined => (
					this.#producers.get(producerId)
				),
				getDataProducerById : (dataProducerId: string): DataProducer | undefined => (
					this.#dataProducers.get(dataProducerId)
				)
			});

		this.#transports.set(transport.id, transport);
		transport.on('@close', () => this.#transports.delete(transport.id));
		transport.on('@listenserverclose', () => this.#transports.delete(transport.id));
		transport.on('@newproducer', (producer: Producer) => this.#producers.set(producer.id, producer));
		transport.on('@producerclose', (producer: Producer) => this.#producers.delete(producer.id));
		transport.on('@newdataproducer', (dataProducer: DataProducer) => (
			this.#dataProducers.set(dataProducer.id, dataProducer)
		));
		transport.on('@dataproducerclose', (dataProducer: DataProducer) => (
			this.#dataProducers.delete(dataProducer.id)
		));

		// Emit observer event.
		this.#observer.safeEmit('newtransport', transport);

		return transport;
	}

	/**
	 * Create a DirectTransport.
	 */
	async createDirectTransport(
		{
			maxMessageSize = 262144,
			appData
		}: DirectTransportOptions =
		{
			maxMessageSize : 262144
		}
	): Promise<DirectTransport>
	{
		logger.debug('createDirectTransport()');

		if (typeof maxMessageSize !== 'number' || maxMessageSize < 0)
		{
			throw new TypeError('if given, maxMessageSize must be a positive number');
		}
		else if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		const transportId = uuidv4();

		/* Build Request. */

		const baseTransportOptions = new FbsTransport.OptionsT(
			true /* direct */,
			maxMessageSize,
			undefined /* initialAvailableOutgoingBitrate */,
			undefined /* enableSctp */,
			undefined /* numSctpStreams */,
			undefined /* maxSctpMessageSize */,
			undefined /* sctpSendBufferSize */,
			undefined /* isDataChannel */
		);

		const directTransportOptions = new FbsDirectTransport.DirectTransportOptionsT(
			baseTransportOptions
		);

		const requestOffset = new FbsRouter.CreateDirectTransportRequestT(
			transportId, directTransportOptions
		).pack(this.#channel.bufferBuilder);

		const response = await this.#channel.request(
			FbsRequest.Method.ROUTER_CREATE_DIRECT_TRANSPORT,
			FbsRequest.Body.FBS_Router_CreateDirectTransportRequest,
			requestOffset,
			this.#internal.routerId
		);

		/* Decode the response. */

		const data = new FbsDirectTransport.DumpResponse();

		response.body(data);

		const directTransportData = parseDirectTransportDumpResponse(data);

		const transport = new DirectTransport(
			{
				internal :
				{
					...this.#internal,
					transportId : transportId
				},
				data                     : directTransportData,
				channel                  : this.#channel,
				appData,
				getRouterRtpCapabilities : (): RtpCapabilities => this.#data.rtpCapabilities,
				getProducerById          : (producerId: string): Producer | undefined => (
					this.#producers.get(producerId)
				),
				getDataProducerById : (dataProducerId: string): DataProducer | undefined => (
					this.#dataProducers.get(dataProducerId)
				)
			});

		this.#transports.set(transport.id, transport);
		transport.on('@close', () => this.#transports.delete(transport.id));
		transport.on('@listenserverclose', () => this.#transports.delete(transport.id));
		transport.on('@newproducer', (producer: Producer) => this.#producers.set(producer.id, producer));
		transport.on('@producerclose', (producer: Producer) => this.#producers.delete(producer.id));
		transport.on('@newdataproducer', (dataProducer: DataProducer) => (
			this.#dataProducers.set(dataProducer.id, dataProducer)
		));
		transport.on('@dataproducerclose', (dataProducer: DataProducer) => (
			this.#dataProducers.delete(dataProducer.id)
		));

		// Emit observer event.
		this.#observer.safeEmit('newtransport', transport);

		return transport;
	}

	/**
	 * Pipes the given Producer or DataProducer into another Router in same host.
	 */
	async pipeToRouter(
		{
			producerId,
			dataProducerId,
			router,
			listenIp = '127.0.0.1',
			enableSctp = true,
			numSctpStreams = { OS: 1024, MIS: 1024 },
			enableRtx = false,
			enableSrtp = false
		}: PipeToRouterOptions
	): Promise<PipeToRouterResult>
	{
		logger.debug('pipeToRouter()');

		if (!producerId && !dataProducerId)
		{
			throw new TypeError('missing producerId or dataProducerId');
		}
		else if (producerId && dataProducerId)
		{
			throw new TypeError('just producerId or dataProducerId can be given');
		}
		else if (!router)
		{
			throw new TypeError('Router not found');
		}
		else if (router === this)
		{
			throw new TypeError('cannot use this Router as destination');
		}

		let producer: Producer | undefined;
		let dataProducer: DataProducer | undefined;

		if (producerId)
		{
			producer = this.#producers.get(producerId);

			if (!producer)
			{
				throw new TypeError('Producer not found');
			}
		}
		else if (dataProducerId)
		{
			dataProducer = this.#dataProducers.get(dataProducerId);

			if (!dataProducer)
			{
				throw new TypeError('DataProducer not found');
			}
		}

		const pipeTransportPairKey = router.id;
		let pipeTransportPairPromise =
			this.#mapRouterPairPipeTransportPairPromise.get(pipeTransportPairKey);
		let pipeTransportPair: PipeTransportPair;
		let localPipeTransport: PipeTransport;
		let remotePipeTransport: PipeTransport;

		if (pipeTransportPairPromise)
		{
			pipeTransportPair = await pipeTransportPairPromise;
			localPipeTransport = pipeTransportPair[this.id];
			remotePipeTransport = pipeTransportPair[router.id];
		}
		else
		{
			pipeTransportPairPromise = new Promise((resolve, reject) =>
			{
				Promise.all(
					[
						this.createPipeTransport(
							{ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp }),
						router.createPipeTransport(
							{ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp })
					])
					.then((pipeTransports) =>
					{
						localPipeTransport = pipeTransports[0];
						remotePipeTransport = pipeTransports[1];
					})
					.then(() =>
					{
						return Promise.all(
							[
								localPipeTransport.connect(
									{
										ip             : remotePipeTransport.tuple.localIp,
										port           : remotePipeTransport.tuple.localPort,
										srtpParameters : remotePipeTransport.srtpParameters
									}),
								remotePipeTransport.connect(
									{
										ip             : localPipeTransport.tuple.localIp,
										port           : localPipeTransport.tuple.localPort,
										srtpParameters : localPipeTransport.srtpParameters
									})
							]);
					})
					.then(() =>
					{
						localPipeTransport.observer.on('close', () =>
						{
							remotePipeTransport.close();
							this.#mapRouterPairPipeTransportPairPromise.delete(
								pipeTransportPairKey);
						});

						remotePipeTransport.observer.on('close', () =>
						{
							localPipeTransport.close();
							this.#mapRouterPairPipeTransportPairPromise.delete(
								pipeTransportPairKey);
						});

						resolve(
							{
								[this.id]   : localPipeTransport,
								[router.id] : remotePipeTransport
							});
					})
					.catch((error) =>
					{
						logger.error(
							'pipeToRouter() | error creating PipeTransport pair:%o',
							error);

						if (localPipeTransport)
						{
							localPipeTransport.close();
						}

						if (remotePipeTransport)
						{
							remotePipeTransport.close();
						}

						reject(error);
					});
			});

			this.#mapRouterPairPipeTransportPairPromise.set(
				pipeTransportPairKey, pipeTransportPairPromise);

			router.addPipeTransportPair(this.id, pipeTransportPairPromise);

			await pipeTransportPairPromise;
		}

		if (producer)
		{
			let pipeConsumer: Consumer | undefined;
			let pipeProducer: Producer | undefined;

			try
			{
				pipeConsumer = await localPipeTransport!.consume(
					{
						producerId : producerId!
					});

				pipeProducer = await remotePipeTransport!.produce(
					{
						id            : producer.id,
						kind          : pipeConsumer!.kind,
						rtpParameters : pipeConsumer!.rtpParameters,
						paused        : pipeConsumer!.producerPaused,
						appData       : producer.appData
					});

				// Ensure that the producer has not been closed in the meanwhile.
				if (producer.closed)
				{
					throw new InvalidStateError('original Producer closed');
				}

				// Ensure that producer.paused has not changed in the meanwhile and, if
				// so, sync the pipeProducer.
				if (pipeProducer.paused !== producer.paused)
				{
					if (producer.paused)
					{
						await pipeProducer.pause();
					}
					else
					{
						await pipeProducer.resume();
					}
				}

				// Pipe events from the pipe Consumer to the pipe Producer.
				pipeConsumer!.observer.on('close', () => pipeProducer!.close());
				pipeConsumer!.observer.on('pause', () => pipeProducer!.pause());
				pipeConsumer!.observer.on('resume', () => pipeProducer!.resume());

				// Pipe events from the pipe Producer to the pipe Consumer.
				pipeProducer.observer.on('close', () => pipeConsumer!.close());

				return { pipeConsumer, pipeProducer };
			}
			catch (error)
			{
				logger.error(
					'pipeToRouter() | error creating pipe Consumer/Producer pair:%o',
					error);

				if (pipeConsumer)
				{
					pipeConsumer.close();
				}

				if (pipeProducer)
				{
					pipeProducer.close();
				}

				throw error;
			}
		}
		else if (dataProducer)
		{
			let pipeDataConsumer: DataConsumer | undefined;
			let pipeDataProducer: DataProducer | undefined;

			try
			{
				pipeDataConsumer = await localPipeTransport!.consumeData(
					{
						dataProducerId : dataProducerId!
					});

				pipeDataProducer = await remotePipeTransport!.produceData(
					{
						id                   : dataProducer.id,
						sctpStreamParameters : pipeDataConsumer!.sctpStreamParameters,
						label                : pipeDataConsumer!.label,
						protocol             : pipeDataConsumer!.protocol,
						appData              : dataProducer.appData
					});

				// Ensure that the dataProducer has not been closed in the meanwhile.
				if (dataProducer.closed)
				{
					throw new InvalidStateError('original DataProducer closed');
				}

				// Pipe events from the pipe DataConsumer to the pipe DataProducer.
				pipeDataConsumer!.observer.on('close', () => pipeDataProducer!.close());

				// Pipe events from the pipe DataProducer to the pipe DataConsumer.
				pipeDataProducer.observer.on('close', () => pipeDataConsumer!.close());

				return { pipeDataConsumer, pipeDataProducer };
			}
			catch (error)
			{
				logger.error(
					'pipeToRouter() | error creating pipe DataConsumer/DataProducer pair:%o',
					error);

				if (pipeDataConsumer)
				{
					pipeDataConsumer.close();
				}

				if (pipeDataProducer)
				{
					pipeDataProducer.close();
				}

				throw error;
			}
		}
		else
		{
			throw new Error('internal error');
		}
	}

	/**
	 * @private
	 */
	addPipeTransportPair(
		pipeTransportPairKey: string,
		pipeTransportPairPromise: Promise<PipeTransportPair>
	): void
	{
		if (this.#mapRouterPairPipeTransportPairPromise.has(pipeTransportPairKey))
		{
			throw new Error(
				'given pipeTransportPairKey already exists in this Router');
		}

		this.#mapRouterPairPipeTransportPairPromise.set(
			pipeTransportPairKey, pipeTransportPairPromise);

		pipeTransportPairPromise
			.then((pipeTransportPair) =>
			{
				const localPipeTransport = pipeTransportPair[this.id];

				// NOTE: No need to do any other cleanup here since that is done by the
				// Router calling this method on us.
				localPipeTransport.observer.on('close', () =>
				{
					this.#mapRouterPairPipeTransportPairPromise.delete(
						pipeTransportPairKey);
				});
			})
			.catch(() =>
			{
				this.#mapRouterPairPipeTransportPairPromise.delete(
					pipeTransportPairKey);
			});
	}

	/**
	 * Create an ActiveSpeakerObserver
	 */
	async createActiveSpeakerObserver(
		{
			interval = 300,
			appData
		}: ActiveSpeakerObserverOptions = {}
	): Promise<ActiveSpeakerObserver>
	{
		logger.debug('createActiveSpeakerObserver()');

		if (typeof interval !== 'number')
		{
			throw new TypeError('if given, interval must be an number');
		}
		if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		const rtpObserverId = uuidv4();

		/* Build Request. */

		const activeRtpObserverOptions =
			new FbsActiveSpeakerObserver.ActiveSpeakerObserverOptionsT(
				interval
			);

		const requestOffset =
			new FbsRouter.CreateActiveSpeakerObserverRequestT(
				rtpObserverId,
				activeRtpObserverOptions
			).pack(this.#channel.bufferBuilder);

		await this.#channel.request(
			FbsRequest.Method.ROUTER_CREATE_ACTIVE_SPEAKER_OBSERVER,
			FbsRequest.Body.FBS_Router_CreateActiveSpeakerObserverRequest,
			requestOffset,
			this.#internal.routerId
		);

		const activeSpeakerObserver = new ActiveSpeakerObserver(
			{
				internal :
				{
					...this.#internal,
					rtpObserverId : rtpObserverId
				},
				channel         : this.#channel,
				appData,
				getProducerById : (producerId: string): Producer | undefined => (
					this.#producers.get(producerId)
				)
			});

		this.#rtpObservers.set(activeSpeakerObserver.id, activeSpeakerObserver);
		activeSpeakerObserver.on('@close', () =>
		{
			this.#rtpObservers.delete(activeSpeakerObserver.id);
		});

		// Emit observer event.
		this.#observer.safeEmit('newrtpobserver', activeSpeakerObserver);

		return activeSpeakerObserver;
	}

	/**
	 * Create an AudioLevelObserver.
	 */
	async createAudioLevelObserver(
		{
			maxEntries = 1,
			threshold = -80,
			interval = 1000,
			appData
		}: AudioLevelObserverOptions = {}
	): Promise<AudioLevelObserver>
	{
		logger.debug('createAudioLevelObserver()');

		if (typeof maxEntries !== 'number' || maxEntries <= 0)
		{
			throw new TypeError('if given, maxEntries must be a positive number');
		}
		if (typeof threshold !== 'number' || threshold < -127 || threshold > 0)
		{
			throw new TypeError('if given, threshole must be a negative number greater than -127');
		}
		if (typeof interval !== 'number')
		{
			throw new TypeError('if given, interval must be an number');
		}
		if (appData && typeof appData !== 'object')
		{
			throw new TypeError('if given, appData must be an object');
		}

		const rtpObserverId = uuidv4();

		/* Build Request. */

		const audioLevelObserverOptions =
			new FbsAudioLevelObserver.AudioLevelObserverOptionsT(
				maxEntries,
				threshold,
				interval
			);

		const requestOffset = new FbsRouter.CreateAudioLevelObserverRequestT(
			rtpObserverId,
			audioLevelObserverOptions
		).pack(this.#channel.bufferBuilder);

		await this.#channel.request(
			FbsRequest.Method.ROUTER_CREATE_AUDIO_LEVEL_OBSERVER,
			FbsRequest.Body.FBS_Router_CreateAudioLevelObserverRequest,
			requestOffset,
			this.#internal.routerId
		);

		const audioLevelObserver = new AudioLevelObserver(
			{
				internal :
				{
					...this.#internal,
					rtpObserverId : rtpObserverId
				},
				channel         : this.#channel,
				appData,
				getProducerById : (producerId: string): Producer | undefined => (
					this.#producers.get(producerId)
				)
			});

		this.#rtpObservers.set(audioLevelObserver.id, audioLevelObserver);
		audioLevelObserver.on('@close', () =>
		{
			this.#rtpObservers.delete(audioLevelObserver.id);
		});

		// Emit observer event.
		this.#observer.safeEmit('newrtpobserver', audioLevelObserver);

		return audioLevelObserver;
	}

	/**
	 * Check whether the given RTP capabilities can consume the given Producer.
	 */
	canConsume(
		{
			producerId,
			rtpCapabilities
		}:
		{
			producerId: string;
			rtpCapabilities: RtpCapabilities;
		}
	): boolean
	{
		const producer = this.#producers.get(producerId);

		if (!producer)
		{
			logger.error(
				'canConsume() | Producer with id "%s" not found', producerId);

			return false;
		}

		try
		{
			return ortc.canConsume(producer.consumableRtpParameters, rtpCapabilities);
		}
		catch (error)
		{
			logger.error('canConsume() | unexpected error: %s', String(error));

			return false;
		}
	}
}
