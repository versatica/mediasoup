import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { TransportInternal } from './Transport';
import { parseSctpStreamParameters, SctpStreamParameters } from './SctpParameters';
import { Event, Notification } from './fbs/notification_generated';
import * as FbsTransport from './fbs/transport_generated';
import * as FbsRequest from './fbs/request_generated';
import * as FbsDataConsumer from './fbs/dataConsumer_generated';

export type DataConsumerOptions =
{
	/**
	 * The id of the DataProducer to consume.
	 */
	dataProducerId: string;

	/**
	 * Just if consuming over SCTP.
	 * Whether data messages must be received in order. If true the messages will
	 * be sent reliably. Defaults to the value in the DataProducer if it has type
	 * 'sctp' or to true if it has type 'direct'.
	 */
	ordered?: boolean;

	/**
	 * Just if consuming over SCTP.
	 * When ordered is false indicates the time (in milliseconds) after which a
	 * SCTP packet will stop being retransmitted. Defaults to the value in the
	 * DataProducer if it has type 'sctp' or unset if it has type 'direct'.
	 */
	maxPacketLifeTime?: number;

	/**
	 * Just if consuming over SCTP.
	 * When ordered is false indicates the maximum number of times a packet will
	 * be retransmitted. Defaults to the value in the DataProducer if it has type
	 * 'sctp' or unset if it has type 'direct'.
	 */
	maxRetransmits?: number;

	/**
	 * Custom application data.
	 */
	appData?: Record<string, unknown>;
};

export type DataConsumerStat =
{
	type: string;
	timestamp: number;
	label: string;
	protocol: string;
	messagesSent: number;
	bytesSent: number;
	bufferedAmount: number;
};

/**
 * DataConsumer type.
 */
export type DataConsumerType = 'sctp' | 'direct';

export type DataConsumerEvents =
{
	transportclose: [];
	dataproducerclose: [];
	message: [Buffer, number];
	sctpsendbufferfull: [];
	bufferedamountlow: [number];
	// Private events.
	'@close': [];
	'@dataproducerclose': [];
};

export type DataConsumerObserverEvents =
{
	close: [];
};

type DataConsumerDump = DataConsumerData & {
	id: string;
};

type DataConsumerInternal = TransportInternal &
{
	dataConsumerId: string;
};

type DataConsumerData =
{
	dataProducerId: string;
	type: DataConsumerType;
	sctpStreamParameters?: SctpStreamParameters;
	label: string;
	protocol: string;
};

const logger = new Logger('DataConsumer');

export class DataConsumer extends EnhancedEventEmitter<DataConsumerEvents>
{
	// Internal data.
	readonly #internal: DataConsumerInternal;

	// DataConsumer data.
	readonly #data: DataConsumerData;

	// Channel instance.
	readonly #channel: Channel;

	// Closed flag.
	#closed = false;

	// Custom app data.
	readonly #appData: Record<string, unknown>;

	// Observer instance.
	readonly #observer = new EnhancedEventEmitter<DataConsumerObserverEvents>();

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
			internal: DataConsumerInternal;
			data: DataConsumerData;
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

		this.handleWorkerNotifications();
	}

	/**
	 * DataConsumer id.
	 */
	get id(): string
	{
		return this.#internal.dataConsumerId;
	}

	/**
	 * Associated DataProducer id.
	 */
	get dataProducerId(): string
	{
		return this.#data.dataProducerId;
	}

	/**
	 * Whether the DataConsumer is closed.
	 */
	get closed(): boolean
	{
		return this.#closed;
	}

	/**
	 * DataConsumer type.
	 */
	get type(): DataConsumerType
	{
		return this.#data.type;
	}

	/**
	 * SCTP stream parameters.
	 */
	get sctpStreamParameters(): SctpStreamParameters | undefined
	{
		return this.#data.sctpStreamParameters;
	}

	/**
	 * DataChannel label.
	 */
	get label(): string
	{
		return this.#data.label;
	}

	/**
	 * DataChannel protocol.
	 */
	get protocol(): string
	{
		return this.#data.protocol;
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
	get observer(): EnhancedEventEmitter<DataConsumerObserverEvents>
	{
		return this.#observer;
	}

	/**
	 * Close the DataConsumer.
	 */
	close(): void
	{
		if (this.#closed)
			return;

		logger.debug('close()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.#channel.removeAllListeners(this.#internal.dataConsumerId);

		/* Build Request. */

		const requestOffset = new FbsTransport.CloseDataConsumerRequestT(
			this.#internal.dataConsumerId
		).pack(this.#channel.bufferBuilder);

		this.#channel.request(
			FbsRequest.Method.TRANSPORT_CLOSE_DATA_CONSUMER,
			FbsRequest.Body.FBS_Transport_CloseDataConsumerRequest,
			requestOffset,
			this.#internal.transportId
		).catch(() => {});

		this.emit('@close');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	transportClosed(): void
	{
		if (this.#closed)
			return;

		logger.debug('transportClosed()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.#channel.removeAllListeners(this.#internal.dataConsumerId);

		this.safeEmit('transportclose');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Dump DataConsumer.
	 */
	async dump(): Promise<DataConsumerDump>
	{
		logger.debug('dump()');

		const response = await this.#channel.request(
			FbsRequest.Method.DATA_CONSUMER_DUMP,
			undefined,
			undefined,
			this.#internal.dataConsumerId
		);

		/* Decode the response. */
		const dumpResponse = new FbsDataConsumer.DumpResponse();

		response.body(dumpResponse);

		return parseDataConsumerDump(dumpResponse);
	}

	/**
	 * Get DataConsumer stats.
	 */
	async getStats(): Promise<DataConsumerStat[]>
	{
		logger.debug('getStats()');

		const response = await this.#channel.request(
			FbsRequest.Method.DATA_CONSUMER_GET_STATS,
			undefined,
			undefined,
			this.#internal.dataConsumerId
		);

		/* Decode the response. */
		const data = new FbsDataConsumer.GetStatsResponse();

		response.body(data);

		return [ parseDataConsumerStats(data) ];
	}

	/**
	 * Set buffered amount low threshold.
	 */
	async setBufferedAmountLowThreshold(threshold: number): Promise<void>
	{
		logger.debug('setBufferedAmountLowThreshold() [threshold:%s]', threshold);

		/* Build Request. */

		const requestOffset = FbsDataConsumer.SetBufferedAmountLowThresholdRequest.
			createSetBufferedAmountLowThresholdRequest(this.#channel.bufferBuilder, threshold);

		await this.#channel.request(
			FbsRequest.Method.DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD,
			FbsRequest.Body.FBS_DataConsumer_SetBufferedAmountLowThresholdRequest,
			requestOffset,
			this.#internal.dataConsumerId
		);
	}

	/**
	 * Send data.
	 */
	async send(message: string | Buffer, ppid?: number): Promise<void>
	{
		if (typeof message !== 'string' && !Buffer.isBuffer(message))
		{
			throw new TypeError('message must be a string or a Buffer');
		}

		/*
		 * +-------------------------------+----------+
		 * | Value                         | SCTP     |
		 * |                               | PPID     |
		 * +-------------------------------+----------+
		 * | WebRTC String                 | 51       |
		 * | WebRTC Binary Partial         | 52       |
		 * | (Deprecated)                  |          |
		 * | WebRTC Binary                 | 53       |
		 * | WebRTC String Partial         | 54       |
		 * | (Deprecated)                  |          |
		 * | WebRTC String Empty           | 56       |
		 * | WebRTC Binary Empty           | 57       |
		 * +-------------------------------+----------+
		 */

		if (typeof ppid !== 'number')
		{
			ppid = (typeof message === 'string')
				? message.length > 0 ? 51 : 56
				: message.length > 0 ? 53 : 57;
		}

		// Ensure we honor PPIDs.
		if (ppid === 56)
			message = ' ';
		else if (ppid === 57)
			message = Buffer.alloc(1);

		const builder = this.#channel.bufferBuilder;

		let dataOffset = 0;

		if (typeof message === 'string')
		{
			const messageOffset = builder.createString(message);

			dataOffset = FbsDataConsumer.String.createString(builder, messageOffset);
		}
		else
		{
			const messageOffset = FbsDataConsumer.Binary.createValueVector(builder, message);

			dataOffset = FbsDataConsumer.Binary.createBinary(builder, messageOffset);
		}

		const requestOffset = FbsDataConsumer.SendRequest.createSendRequest(
			builder,
			ppid,
			typeof message === 'string' ?
				FbsDataConsumer.Data.String :
				FbsDataConsumer.Data.Binary,
			dataOffset
		);

		await this.#channel.request(
			FbsRequest.Method.DATA_CONSUMER_SEND,
			FbsRequest.Body.FBS_DataConsumer_SendRequest,
			requestOffset,
			this.#internal.dataConsumerId
		);
	}

	/**
	 * Get buffered amount size.
	 */
	async getBufferedAmount(): Promise<number>
	{
		logger.debug('getBufferedAmount()');

		const response = await this.#channel.request(
			FbsRequest.Method.DATA_CONSUMER_GET_BUFFERED_AMOUNT,
			undefined,
			undefined,
			this.#internal.dataConsumerId
		);

		const data = new FbsDataConsumer.GetBufferedAmountResponse();

		response.body(data);

		return data.bufferedAmount();
	}

	private handleWorkerNotifications(): void
	{
		this.#channel.on(this.#internal.dataConsumerId, (event: Event, data?: Notification) =>
		{
			switch (event)
			{
				case Event.DATACONSUMER_DATAPRODUCER_CLOSE:
				{
					if (this.#closed)
						break;

					this.#closed = true;

					// Remove notification subscriptions.
					this.#channel.removeAllListeners(this.#internal.dataConsumerId);

					this.emit('@dataproducerclose');
					this.safeEmit('dataproducerclose');

					// Emit observer event.
					this.#observer.safeEmit('close');

					break;
				}

				case Event.DATACONSUMER_SCTP_SENDBUFFER_FULL:
				{
					this.safeEmit('sctpsendbufferfull');

					break;
				}

				case Event.DATACONSUMER_BUFFERED_AMOUNT_LOW:
				{
					const notification = new FbsDataConsumer.BufferedAmountLowNotification();

					data!.body(notification);

					const bufferedAmount = notification.bufferedAmount();

					this.safeEmit('bufferedamountlow', bufferedAmount);

					break;
				}

				case Event.DATACONSUMER_MESSAGE:
				{
					if (this.#closed)
						break;

					const notification = new FbsDataConsumer.MessageNotification();

					data!.body(notification);

					this.safeEmit(
						'message',
						Buffer.from(notification.dataArray()!),
						notification.ppid()
					);

					break;
				}

				default:
				{
					logger.error('ignoring unknown event "%s" in channel listener', event);
				}
			}
		});
	}
}

export function parseDataConsumerDump(
	data: FbsDataConsumer.DumpResponse
): DataConsumerDump
{
	return {
		id                   : data.id()!,
		dataProducerId       : data.dataProducerId()!,
		type                 : data.type()! as DataConsumerType,
		sctpStreamParameters : data.sctpStreamParameters() !== null ?
			parseSctpStreamParameters(data.sctpStreamParameters()!) :
			undefined,
		label    : data.label()!,
		protocol : data.protocol()!
	};
}

function parseDataConsumerStats(
	binary: FbsDataConsumer.GetStatsResponse
):DataConsumerStat
{
	return {
		type           : 'data-consumer',
		timestamp      : Number(binary.timestamp()),
		label          : binary.label()!,
		protocol       : binary.protocol()!,
		messagesSent   : Number(binary.messagesSent()),
		bytesSent      : Number(binary.bytesSent()),
		bufferedAmount : binary.bufferedAmount()
	};
}
