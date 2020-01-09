import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { SctpStreamParameters } from './SctpParameters';

export interface DataConsumerOptions
{
	/**
	 * The id of the DataProducer to consume.
	 */
	dataProducerId: string;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

export interface DataConsumerStat
{
	type: string;
	timestamp: number;
	label: string;
	protocol: string;
	messagesSent: number;
	bytesSent: number;
}

const logger = new Logger('DataConsumer');

export class DataConsumer extends EnhancedEventEmitter
{
	// Internal data.
	// - .routerId
	// - .transportId
	// - .dataConsumerId
	// - .dataProducerId
	private readonly _internal: any;

	// DataConsumer data.
	// - .sctpStreamParameters
	// - .label
	// - .protocol
	private readonly _data: any;

	// Channel instance.
	private readonly _channel: Channel;

	// Closed flag.
	private _closed = false;

	// Custom app data.
	private readonly _appData?: any;

	// Observer instance.
	private readonly _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @emits transportclose
	 * @emits dataproducerclose
	 * @emits @close
	 * @emits @dataproducerclose
	 */
	constructor(
		{
			internal,
			data,
			channel,
			appData
		}:
		{
			internal: any;
			data: any;
			channel: Channel;
			appData: any;
		}
	)
	{
		super(logger);

		logger.debug('constructor()');

		this._internal = internal;
		this._data = data;
		this._channel = channel;
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
	 * Whether the DataConsumer is closed.
	 */
	get closed(): boolean
	{
		return this._closed;
	}

	/**
	 * SCTP stream parameters.
	 */
	get sctpStreamParameters(): SctpStreamParameters
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

	private _handleWorkerNotifications(): void
	{
		this._channel.on(this._internal.dataConsumerId, (event: string) =>
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

				default:
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});
	}
}
