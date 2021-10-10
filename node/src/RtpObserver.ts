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
	protected readonly internal:
	{
		routerId: string;
		rtpObserverId: string;
	};

	// Channel instance.
	protected readonly channel: Channel;

	// PayloadChannel instance.
	protected readonly payloadChannel: PayloadChannel;

	// Closed flag.
	#closed = false;

	// Paused flag.
	#paused = false;

	// Custom app data.
	readonly #appData?: any;

	// Method to retrieve a Producer.
	protected readonly getProducerById: (producerId: string) => Producer;

	// Observer instance.
	readonly #observer = new EnhancedEventEmitter();

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

		this.internal = internal;
		this.channel = channel;
		this.payloadChannel = payloadChannel;
		this.#appData = appData;
		this.getProducerById = getProducerById;
	}

	/**
	 * RtpObserver id.
	 */
	get id(): string
	{
		return this.internal.rtpObserverId;
	}

	/**
	 * Whether the RtpObserver is closed.
	 */
	get closed(): boolean
	{
		return this.#closed;
	}

	/**
	 * Whether the RtpObserver is paused.
	 */
	get paused(): boolean
	{
		return this.#paused;
	}

	/**
	 * App custom data.
	 */
	get appData(): any
	{
		return this.#appData;
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
		return this.#observer;
	}

	/**
	 * Close the RtpObserver.
	 */
	close(): void
	{
		if (this.#closed)
			return;

		logger.debug('close()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.channel.removeAllListeners(this.internal.rtpObserverId);
		this.payloadChannel.removeAllListeners(this.internal.rtpObserverId);

		this.channel.request('rtpObserver.close', this.internal)
			.catch(() => {});

		this.emit('@close');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 */
	routerClosed(): void
	{
		if (this.#closed)
			return;

		logger.debug('routerClosed()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.channel.removeAllListeners(this.internal.rtpObserverId);
		this.payloadChannel.removeAllListeners(this.internal.rtpObserverId);

		this.safeEmit('routerclose');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Pause the RtpObserver.
	 */
	async pause(): Promise<void>
	{
		logger.debug('pause()');

		const wasPaused = this.#paused;

		await this.channel.request('rtpObserver.pause', this.internal);

		this.#paused = true;

		// Emit observer event.
		if (!wasPaused)
			this.#observer.safeEmit('pause');
	}

	/**
	 * Resume the RtpObserver.
	 */
	async resume(): Promise<void>
	{
		logger.debug('resume()');

		const wasPaused = this.#paused;

		await this.channel.request('rtpObserver.resume', this.internal);

		this.#paused = false;

		// Emit observer event.
		if (wasPaused)
			this.#observer.safeEmit('resume');
	}

	/**
	 * Add a Producer to the RtpObserver.
	 */
	async addProducer({ producerId }: RtpObserverAddRemoveProducerOptions): Promise<void>
	{
		logger.debug('addProducer()');

		const producer = this.getProducerById(producerId);
		const reqData = { producerId };

		await this.channel.request('rtpObserver.addProducer', this.internal, reqData);

		// Emit observer event.
		this.#observer.safeEmit('addproducer', producer);
	}

	/**
	 * Remove a Producer from the RtpObserver.
	 */
	async removeProducer({ producerId }: RtpObserverAddRemoveProducerOptions): Promise<void>
	{
		logger.debug('removeProducer()');

		const producer = this.getProducerById(producerId);
		const reqData = { producerId };

		await this.channel.request('rtpObserver.removeProducer', this.internal, reqData);

		// Emit observer event.
		this.#observer.safeEmit('removeproducer', producer);
	}
}
