import uuidv4 from 'uuid/v4';
import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import * as ortc from './ortc';
import {
	Transport,
	TransportListenIp,
	TransportTuple,
	TransportTraceEventData,
	SctpState
} from './Transport';
import { Consumer, ConsumerOptions } from './Consumer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';

export interface PipeTransportOptions
{
	/**
	 * Listening IP address.
	 */
	listenIp: TransportListenIp | string;

	/**
	 * Create a SCTP association. Default false.
	 */
	enableSctp?: boolean;

	/**
	 * SCTP streams number.
	 */
	numSctpStreams?: NumSctpStreams;

	/**
	 * Maximum size of data that can be passed to DataProducer's send() method.
	 * Default 262144.
	 */
	maxSctpMessageSize?: number;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

export interface PipeTransportStat
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
	probationBytesReceived: number;
	probationRecvBitrate: number;
	probationBytesSent: number;
	probationSendBitrate: number;
	availableOutgoingBitrate?: number;
	availableIncomingBitrate?: number;
	maxIncomingBitrate?: number;

	// PipeTransport specific.
	tuple: TransportTuple;
}

const logger = new Logger('PipeTransport');

export class PipeTransport extends Transport
{
	// PipeTransport data.
	// - .tuple
	//   - .localIp
	//   - .localPort
	//   - .remoteIp
	//   - .remotePort
	//   - .protocol
	// - .sctpParameters
	//   - .port
	//   - .OS
	//   - .MIS
	//   - .maxMessageSize
	// - .sctpState

	/**
	 * @private
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
			tuple          : data.tuple,
			sctpParameters : data.sctpParameters,
			sctpState      : data.sctpState
		};

		this._handleWorkerNotifications();
	}

	/**
	 * Transport tuple.
	 */
	get tuple(): TransportTuple
	{
		return this._data.tuple;
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
	get sctpState(): SctpState
	{
		return this._data.sctpState;
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @emits close
	 * @emits newproducer - (producer: Producer)
	 * @emits newconsumer - (producer: Producer)
	 * @emits newdataproducer - (dataProducer: DataProducer)
	 * @emits newdataconsumer - (dataProducer: DataProducer)
	 * @emits sctpstatechange - (sctpState: SctpState)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	get observer(): EnhancedEventEmitter
	{
		return this._observer;
	}

	/**
	 * Close the PlainRtpTransport.
	 *
	 * @override
	 */
	close(): void
	{
		if (this._closed)
			return;

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

		if (this._data.sctpState)
			this._data.sctpState = 'closed';

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

		return this._channel.request('transport.getStats', this._internal);
	}

	/**
	 * Provide the PipeTransport remote parameters.
	 *
	 * @override
	 */
	async connect(
		{
			ip,
			port
		}:
		{
			ip: string;
			port: number;
		}
	): Promise<void>
	{
		logger.debug('connect()');

		const reqData = { ip, port };

		const data =
			await this._channel.request('transport.connect', this._internal, reqData);

		// Update data.
		this._data.tuple = data.tuple;
	}

	/**
	 * Create a pipe Consumer.
	 *
	 * @override
	 */
	async consume({ producerId, appData = {} }: ConsumerOptions): Promise<Consumer>
	{
		logger.debug('consume()');

		if (!producerId || typeof producerId !== 'string')
			throw new TypeError('missing producerId');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		const producer = this._getProducerById(producerId);

		if (!producer)
			throw Error(`Producer with id "${producerId}" not found`);

		// This may throw.
		const rtpParameters =
			ortc.getPipeConsumerRtpParameters(producer.consumableRtpParameters);

		const internal = { ...this._internal, consumerId: uuidv4(), producerId };
		const reqData =
		{
			kind                   : producer.kind,
			rtpParameters,
			type                   : 'pipe',
			consumableRtpEncodings : producer.consumableRtpParameters.encodings
		};

		const status =
			await this._channel.request('transport.consume', internal, reqData);

		const data = { kind: producer.kind, rtpParameters, type: 'pipe' };

		const consumer = new Consumer(
			{
				internal,
				data,
				channel        : this._channel,
				appData,
				paused         : status.paused,
				producerPaused : status.producerPaused
			});

		this._consumers.set(consumer.id, consumer);
		consumer.on('@close', () => this._consumers.delete(consumer.id));
		consumer.on('@producerclose', () => this._consumers.delete(consumer.id));

		// Emit observer event.
		this._observer.safeEmit('newconsumer', consumer);

		return consumer;
	}

	private _handleWorkerNotifications(): void
	{
		this._channel.on(this._internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
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
