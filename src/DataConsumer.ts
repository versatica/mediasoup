import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { SctpStreamParameters } from './SctpParameters';

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
	appData?: any;
}

export type DataConsumerStat =
{
	type: string;
	timestamp: number;
	label: string;
	protocol: string;
	messagesSent: number;
	bytesSent: number;
}

/**
 * DataConsumer type.
 */
export type DataConsumerType = 'sctp' | 'direct';

const logger = new Logger('DataConsumer');

export class DataConsumer extends EnhancedEventEmitter
{
	// Internal data.
	private readonly _internal:
	{
		routerId: string;
		transportId: string;
		dataProducerId: string;
		dataConsumerId: string;
	};

	// DataConsumer data.
	private readonly _data:
	{
		type: DataConsumerType;
		sctpStreamParameters?: SctpStreamParameters;
		label: string;
		protocol: string;
	};

	// Channel instance.
	private readonly _channel: Channel;

	// PayloadChannel instance.
	private readonly _payloadChannel: PayloadChannel;

	// Closed flag.
	private _closed = false;

	// Buffered amount threshold.
	private _bufferedAmountLowThreshold = 0

	// Buffered amount.
	private _bufferedAmount = 0

	// Custom app data.
	private readonly _appData?: any;

	// Observer instance.
	private readonly _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @emits transportclose
	 * @emits dataproducerclose
	 * @emits message - (message: Buffer, ppid: number)
	 * @emits sctpsendbufferfull
	 * @emits bufferedamountlow - (bufferedAmount: number)
	 * @emits @close
	 * @emits @dataproducerclose
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
			internal: any;
			data: any;
			channel: Channel;
			payloadChannel: PayloadChannel;
			appData: any;
		}
	)
	{
		super();

		logger.debug('constructor()');

		this._internal = internal;
		this._data = data;
		this._channel = channel;
		this._payloadChannel = payloadChannel;
		this._appData = appData;

		this._handleWorkerNotifications();
	}

	/**
	 * DataConsumer id.
	 */
	get id(): string
	{
		return this._internal.dataConsumerId;
	}

	/**
	 * Associated DataProducer id.
	 */
	get dataProducerId(): string
	{
		return this._internal.dataProducerId;
	}

	/**
	 * Buffered amount threshold.
	 */
	get bufferedAmountLowThreshold(): number
	{
		return this._bufferedAmountLowThreshold;
	}

	set bufferedAmountLowThreshold(value: number)
	{
		this._bufferedAmountLowThreshold = value;
	}

	/**
	 * Whether the DataConsumer is closed.
	 */
	get closed(): boolean
	{
		return this._closed;
	}

	/**
	 * DataConsumer type.
	 */
	get type(): DataConsumerType
	{
		return this._data.type;
	}

	/**
	 * SCTP stream parameters.
	 */
	get sctpStreamParameters(): SctpStreamParameters | undefined
	{
		return this._data.sctpStreamParameters;
	}

	/**
	 * DataChannel label.
	 */
	get label(): string
	{
		return this._data.label;
	}

	/**
	 * DataChannel protocol.
	 */
	get protocol(): string
	{
		return this._data.protocol;
	}

	/**
	 * App custom data.
	 */
	get appData(): any
	{
		return this._appData;
	}

	/**
	 * Invalid setter.
	 */
	set appData(appData: any) // eslint-disable-line no-unused-vars
	{
		throw new Error('cannot override appData object');
	}

	/**
	 * Observer.
	 *
	 * @emits close
	 */
	get observer(): EnhancedEventEmitter
	{
		return this._observer;
	}

	/**
	 * Close the DataConsumer.
	 */
	close(): void
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.dataConsumerId);

		this._channel.request('dataConsumer.close', this._internal)
			.catch(() => {});

		this.emit('@close');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	transportClosed(): void
	{
		if (this._closed)
			return;

		logger.debug('transportClosed()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.dataConsumerId);

		this.safeEmit('transportclose');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Dump DataConsumer.
	 */
	async dump(): Promise<any>
	{
		logger.debug('dump()');

		return this._channel.request('dataConsumer.dump', this._internal);
	}

	/**
	 * Get DataConsumer stats.
	 */
	async getStats(): Promise<DataConsumerStat[]>
	{
		logger.debug('getStats()');

		return this._channel.request('dataConsumer.getStats', this._internal);
	}

	/**
	 * Get buffered amount size.
	 */
	async getBufferedAmount(): Promise<number>
	{
		logger.debug('getBufferedAmount()');

		const { bufferedAmount } =
			await this._channel.request('dataConsumer.getBufferedAmount', this._internal);

		return bufferedAmount;
	}

	private _handleWorkerNotifications(): void
	{
		this._channel.on(this._internal.dataConsumerId, (event: string, data: any) =>
		{
			switch (event)
			{
				case 'dataproducerclose':
				{
					if (this._closed)
						break;

					this._closed = true;

					// Remove notification subscriptions.
					this._channel.removeAllListeners(this._internal.dataConsumerId);

					this.emit('@dataproducerclose');
					this.safeEmit('dataproducerclose');

					// Emit observer event.
					this._observer.safeEmit('close');

					break;
				}

				case 'sctpsendbufferfull':
				{
					this.safeEmit('sctpsendbufferfull');

					break;
				}

				case 'bufferedamount':
				{
					const { bufferedAmount } = data as { bufferedAmount: number };
					const previousBufferedAmount = this._bufferedAmount;

					this._bufferedAmount = bufferedAmount;

					if (
						previousBufferedAmount > this._bufferedAmountLowThreshold &&
						this._bufferedAmount <= this._bufferedAmountLowThreshold
					)
					{
						this.safeEmit('bufferedamountlow', bufferedAmount);
					}

					break;
				}

				default:
				{
					logger.error('ignoring unknown event "%s" in channel listener', event);
				}
			}
		});

		this._payloadChannel.on(
			this._internal.dataConsumerId,
			(event: string, data: any | undefined, payload: Buffer) =>
			{
				switch (event)
				{
					case 'message':
					{
						if (this._closed)
							break;

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
