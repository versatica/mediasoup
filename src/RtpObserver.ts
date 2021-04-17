import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { Producer } from './Producer';

const logger = new Logger('RtpObserver');

export type RtpObserverAddRemoveProducerOptions =
{
	/**
	 * The id of the Producer to be added or removed.
	 */
	producerId: string;
}

export class RtpObserver extends EnhancedEventEmitter
{
	// Internal data.
	protected readonly _internal:
	{
		routerId: string;
		rtpObserverId: string;
	};

	// Channel instance.
	protected readonly _channel: Channel;

	// PayloadChannel instance.
	protected readonly _payloadChannel: PayloadChannel;

	// Closed flag.
	protected _closed = false;

	// Paused flag.
	protected _paused = false;

	// Custom app data.
	private readonly _appData?: any;

	// Method to retrieve a Producer.
	protected readonly _getProducerById: (producerId: string) => Producer;

	// Observer instance.
	protected readonly _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @interface
	 * @emits routerclose
	 * @emits @close
	 */
	constructor(
		{
			internal,
			channel,
			payloadChannel,
			appData,
			getProducerById
		}:
		{
			internal: any;
			channel: Channel;
			payloadChannel: PayloadChannel;
			appData: any;
			getProducerById: (producerId: string) => Producer;
		}
	)
	{
		super();

		logger.debug('constructor()');

		this._internal = internal;
		this._channel = channel;
		this._payloadChannel = payloadChannel;
		this._appData = appData;
		this._getProducerById = getProducerById;
	}

	/**
	 * RtpObserver id.
	 */
	get id(): string
	{
		return this._internal.rtpObserverId;
	}

	/**
	 * Whether the RtpObserver is closed.
	 */
	get closed(): boolean
	{
		return this._closed;
	}

	/**
	 * Whether the RtpObserver is paused.
	 */
	get paused(): boolean
	{
		return this._paused;
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
	 * @emits pause
	 * @emits resume
	 * @emits addproducer - (producer: Producer)
	 * @emits removeproducer - (producer: Producer)
	 */
	get observer(): EnhancedEventEmitter
	{
		return this._observer;
	}

	/**
	 * Close the RtpObserver.
	 */
	close(): void
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.rtpObserverId);
		this._payloadChannel.removeAllListeners(this._internal.rtpObserverId);

		this._channel.request('rtpObserver.close', this._internal)
			.catch(() => {});

		this.emit('@close');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 */
	routerClosed(): void
	{
		if (this._closed)
			return;

		logger.debug('routerClosed()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.rtpObserverId);
		this._payloadChannel.removeAllListeners(this._internal.rtpObserverId);

		this.safeEmit('routerclose');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Pause the RtpObserver.
	 */
	async pause(): Promise<void>
	{
		logger.debug('pause()');

		const wasPaused = this._paused;

		await this._channel.request('rtpObserver.pause', this._internal);

		this._paused = true;

		// Emit observer event.
		if (!wasPaused)
			this._observer.safeEmit('pause');
	}

	/**
	 * Resume the RtpObserver.
	 */
	async resume(): Promise<void>
	{
		logger.debug('resume()');

		const wasPaused = this._paused;

		await this._channel.request('rtpObserver.resume', this._internal);

		this._paused = false;

		// Emit observer event.
		if (wasPaused)
			this._observer.safeEmit('resume');
	}

	/**
	 * Add a Producer to the RtpObserver.
	 */
	async addProducer({ producerId }: RtpObserverAddRemoveProducerOptions): Promise<void>
	{
		logger.debug('addProducer()');

		const producer = this._getProducerById(producerId);
		const internal = { ...this._internal, producerId };

		await this._channel.request('rtpObserver.addProducer', internal);

		// Emit observer event.
		this._observer.safeEmit('addproducer', producer);
	}

	/**
	 * Remove a Producer from the RtpObserver.
	 */
	async removeProducer({ producerId }: RtpObserverAddRemoveProducerOptions): Promise<void>
	{
		logger.debug('removeProducer()');

		const producer = this._getProducerById(producerId);
		const internal = { ...this._internal, producerId };

		await this._channel.request('rtpObserver.removeProducer', internal);

		// Emit observer event.
		this._observer.safeEmit('removeproducer', producer);
	}
}
