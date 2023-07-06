import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { TransportInternal } from './Transport';
import { SctpStreamParameters } from './SctpParameters';
import { AppData } from './types';

/**
 * SCTP stream parameters. Identical to {@link SctpStreamParameters} with the
 * exception that the `streamId` is optional.
 */
type DataConsumerOptionsSctpStreamParameters = {
	/**
	 * SCTP stream id. If not provided, a free SCTP stream id will be assigned.
	 */
	streamId?: number;
} & Omit<SctpStreamParameters, 'streamId'>;

export type DataConsumerOptions<DataConsumerAppData extends AppData = AppData> =
{
	/**
	 * The id of the DataProducer to consume.
	 */
	dataProducerId: string;

	/**
	 * SCTP parameters defining how the endpoint is sending the data.
	 * Only if messages are sent over SCTP.
	 */
	sctpStreamParameters?: DataConsumerOptionsSctpStreamParameters;

	/**
	 * Custom application data.
	 */
	appData?: DataConsumerAppData;
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

export class DataConsumer<DataConsumerAppData extends AppData = AppData>
	extends EnhancedEventEmitter<DataConsumerEvents>
{
	// Internal data.
	readonly #internal: DataConsumerInternal;

	// DataConsumer data.
	readonly #data: DataConsumerData;

	// Channel instance.
	readonly #channel: Channel;

	// PayloadChannel instance.
	readonly #payloadChannel: PayloadChannel;

	// Closed flag.
	#closed = false;

	// Custom app data.
	#appData: DataConsumerAppData;

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
			payloadChannel,
			appData
		}:
		{
			internal: DataConsumerInternal;
			data: DataConsumerData;
			channel: Channel;
			payloadChannel: PayloadChannel;
			appData?: DataConsumerAppData;
		}
	)
	{
		super();

		logger.debug('constructor()');

		this.#internal = internal;
		this.#data = data;
		this.#channel = channel;
		this.#payloadChannel = payloadChannel;
		this.#appData = appData || {} as DataConsumerAppData;

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
	get appData(): DataConsumerAppData
	{
		return this.#appData;
	}

	/**
	 * App custom data setter.
	 */
	set appData(appData: DataConsumerAppData)
	{
		this.#appData = appData;
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
		{
			return;
		}

		logger.debug('close()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.#channel.removeAllListeners(this.#internal.dataConsumerId);
		this.#payloadChannel.removeAllListeners(this.#internal.dataConsumerId);

		const reqData = { dataConsumerId: this.#internal.dataConsumerId };

		this.#channel.request('transport.closeDataConsumer', this.#internal.transportId, reqData)
			.catch(() => {});

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
		{
			return;
		}

		logger.debug('transportClosed()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.#channel.removeAllListeners(this.#internal.dataConsumerId);
		this.#payloadChannel.removeAllListeners(this.#internal.dataConsumerId);

		this.safeEmit('transportclose');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Dump DataConsumer.
	 */
	async dump(): Promise<any>
	{
		logger.debug('dump()');

		return this.#channel.request('dataConsumer.dump', this.#internal.dataConsumerId);
	}

	/**
	 * Get DataConsumer stats.
	 */
	async getStats(): Promise<DataConsumerStat[]>
	{
		logger.debug('getStats()');

		return this.#channel.request('dataConsumer.getStats', this.#internal.dataConsumerId);
	}

	/**
	 * Set buffered amount low threshold.
	 */
	async setBufferedAmountLowThreshold(threshold: number): Promise<void>
	{
		logger.debug('setBufferedAmountLowThreshold() [threshold:%s]', threshold);

		const reqData = { threshold };

		await this.#channel.request(
			'dataConsumer.setBufferedAmountLowThreshold', this.#internal.dataConsumerId, reqData);
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
		{
			message = ' ';
		}
		else if (ppid === 57)
		{
			message = Buffer.alloc(1);
		}

		const requestData = String(ppid);

		await this.#payloadChannel.request(
			'dataConsumer.send', this.#internal.dataConsumerId, requestData, message);
	}

	/**
	 * Get buffered amount size.
	 */
	async getBufferedAmount(): Promise<number>
	{
		logger.debug('getBufferedAmount()');

		const { bufferedAmount } =
			await this.#channel.request('dataConsumer.getBufferedAmount', this.#internal.dataConsumerId);

		return bufferedAmount;
	}

	private handleWorkerNotifications(): void
	{
		this.#channel.on(this.#internal.dataConsumerId, (event: string, data: any) =>
		{
			switch (event)
			{
				case 'dataproducerclose':
				{
					if (this.#closed)
					{
						break;
					}

					this.#closed = true;

					// Remove notification subscriptions.
					this.#channel.removeAllListeners(this.#internal.dataConsumerId);
					this.#payloadChannel.removeAllListeners(this.#internal.dataConsumerId);

					this.emit('@dataproducerclose');
					this.safeEmit('dataproducerclose');

					// Emit observer event.
					this.#observer.safeEmit('close');

					break;
				}

				case 'sctpsendbufferfull':
				{
					this.safeEmit('sctpsendbufferfull');

					break;
				}

				case 'bufferedamountlow':
				{
					const { bufferedAmount } = data as { bufferedAmount: number };

					this.safeEmit('bufferedamountlow', bufferedAmount);

					break;
				}

				default:
				{
					logger.error('ignoring unknown event "%s" in channel listener', event);
				}
			}
		});

		this.#payloadChannel.on(
			this.#internal.dataConsumerId,
			(event: string, data: any | undefined, payload: Buffer) =>
			{
				switch (event)
				{
					case 'message':
					{
						if (this.#closed)
						{
							break;
						}

						const ppid = data.ppid as number;
						const message = payload;

						this.safeEmit('message', message, ppid);

						break;
					}

					default:
					{
						logger.error('ignoring unknown event "%s" in payload channel listener', event);
					}
				}
			});
	}
}
