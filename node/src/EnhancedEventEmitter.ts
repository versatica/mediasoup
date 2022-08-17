import { EventEmitter } from 'events';
import { Logger } from './Logger';

const logger = new Logger('EnhancedEventEmitter');

type Events = Record<string, any[]>

export class EnhancedEventEmitter<E extends Events = Events> extends EventEmitter
{
	constructor()
	{
		super();
		this.setMaxListeners(Infinity);
	}

	emit<K extends keyof E & string>(eventName: K, ...args: E[K]): boolean
	{
		return super.emit(eventName, ...args);
	}

	/**
	 * Special addition to the EventEmitter API.
	 */
	safeEmit<K extends keyof E & string>(eventName: K, ...args: E[K]): boolean
	{
		const numListeners = super.listenerCount(eventName);

		try
		{
			return super.emit(eventName, ...args);
		}
		catch (error)
		{
			logger.error(
				'safeEmit() | event listener threw an error [eventName:%s]:%o',
				eventName, error);

			return Boolean(numListeners);
		}
	}

	on<K extends keyof E & string>(
		eventName: K,
		listener: (...args: E[K]) => void
	): this
	{
		super.on(eventName, listener as (...args: any[]) => void);

		return this;
	}

	off<K extends keyof E & string>(
		eventName: K,
		listener: (...args: E[K]) => void
	): this
	{
		super.off(eventName, listener as (...args: any[]) => void);

		return this;
	}

	addListener<K extends keyof E & string>(
		eventName: K,
		listener: (...args: E[K]) => void
	): this
	{
		super.on(eventName, listener as (...args: any[]) => void);

		return this;
	}

	prependListener<K extends keyof E & string>(
		eventName: K,
		listener: (...args: E[K]) => void
	): this
	{
		super.prependListener(eventName, listener as (...args: any[]) => void);

		return this;
	}

	once<K extends keyof E & string>(
		eventName: K,
		listener: (...args: E[K]) => void
	): this
	{
		super.once(eventName, listener as (...args: any[]) => void);

		return this;
	}

	prependOnceListener<K extends keyof E & string>(
		eventName: K,
		listener: (...args: E[K]) => void
	): this
	{
		super.prependOnceListener(eventName, listener as (...args: any[]) => void);

		return this;
	}

	removeListener<K extends keyof E & string>(
		eventName: K,
		listener: (...args: E[K]) => void
	): this
	{
		super.off(eventName, listener as (...args: any[]) => void);

		return this;
	}

	removeAllListeners<K extends keyof E & string>(eventName?: K): this
	{
		super.removeAllListeners(eventName);

		return this;
	}

	listenerCount<K extends keyof E & string>(eventName: K): number
	{
		return super.listenerCount(eventName);
	}

	listeners<K extends keyof E & string>(eventName: K): Function[]
	{
		return super.listeners(eventName);
	}

	rawListeners<K extends keyof E & string>(eventName: K): Function[]
	{
		return super.rawListeners(eventName);
	}
}
