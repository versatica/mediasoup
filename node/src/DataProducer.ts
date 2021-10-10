import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { SctpStreamParameters } from './SctpParameters';

export type DataProducerOptions =
{
	/**
	 * DataProducer id (just for Router.pipeToRouter() method).
	 */
	id?: string;

	/**
	 * SCTP parameters defining how the endpoint is sending the data.
	 * Just if messages are sent over SCTP.
	 */
	sctpStreamParameters?: SctpStreamParameters;

	/**
	 * A label which can be used to distinguish this DataChannel from others.
	 */
	label?: string;

	/**
	 * Name of the sub-protocol used by this DataChannel.
	 */
	protocol?: string;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

export type DataProducerStat =
{
	type: string;
	timestamp: number;
	label: string;
	protocol: string;
	messagesReceived: number;
	bytesReceived: number;
}

/**
 * DataProducer type.
 */
export type DataProducerType = 'sctp' | 'direct';

const logger = new Logger('DataProducer');

export class DataProducer extends EnhancedEventEmitter
{
	// Internal data.
	private readonly _internal:
	{
		routerId: string;
		transportId: string;
		dataProducerId: string;
	};

	// DataProducer data.
	private readonly _data:
	{
		type: DataProducerType;
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

	// Custom app data.
	private readonly _appData?: any;

	// Observer instance.
	private readonly _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @emits transportclose
	 * @emits @close
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
	 * DataProducer id.
	 */
	get id(): string
	{
		return this._internal.dataProducerId;
	}

	/**
	 * Whether the DataProducer is closed.
	 */
	get closed(): boolean
	{
		return this._closed;
	}

	/**
	 * DataProducer type.
	 */
	get type(): DataProducerType
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
	 * Close the DataProducer.
	 */
	close(): void
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.dataProducerId);
		this._payloadChannel.removeAllListeners(this._internal.dataProducerId);

		this._channel.request('dataProducer.close', this._internal)
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
		this._channel.removeAllListeners(this._internal.dataProducerId);
		this._payloadChannel.removeAllListeners(this._internal.dataProducerId);

		this.safeEmit('transportclose');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Dump DataProducer.
	 */
	async dump(): Promise<any>
	{
		logger.debug('dump()');

		return this._channel.request('dataProducer.dump', this._internal);
	}

	/**
	 * Get DataProducer stats.
	 */
	async getStats(): Promise<DataProducerStat[]>
	{
		logger.debug('getStats()');

		return this._channel.request('dataProducer.getStats', this._internal);
	}

	/**
	 * Send data (just valid for DataProducers created on a DirectTransport).
	 */
	send(message: string | Buffer, ppid?: number): void
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

		const notifData = { ppid };

		this._payloadChannel.notify(
			'dataProducer.send', this._internal, notifData, message);
	}

	private _handleWorkerNotifications(): void
	{
		// No need to subscribe to any event.
	}
}
