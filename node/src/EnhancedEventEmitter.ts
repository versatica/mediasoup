import { EventEmitter } from 'events';
import { Logger } from './Logger';

const logger = new Logger('EnhancedEventEmitter');

type Events = Record<string, any[]>
type InternalEvents = Record<`@${string}`, any[]>

export class EnhancedEventEmitter<PublicEvents extends Events = Events,
E extends PublicEvents = PublicEvents & InternalEvents> extends EventEmitter
{
	constructor()
	{
		super();
		this.setMaxListeners(Infinity);
	}

	safeEmit<K extends keyof E & string>(event: K, ...args: E[K]): boolean
	{
		const numListeners = this.listenerCount(event);

		try
		{
			return this.emit(event, ...args);
		}
		catch (error)
		{
			logger.error(
				'safeEmit() | event listener threw an error [event:%s]:%o',
				event, error);

			return Boolean(numListeners);
		}
	}

	async safeEmitAsPromise<K extends keyof E & string>(event: K, ...args: E[K]):
	Promise<any>
	{
		return new Promise((resolve, reject) =>
		{
			try
			{
				this.emit(event, ...args, resolve, reject);
			}
			catch (error)
			{
				logger.error(
					'safeEmitAsPromise() | event listener threw an error [event:%s]:%o',
					event, error);

				reject(error);
			}
		});
	}

	on<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void)
	{
		super.on(event, listener as (...args: any[]) => void);

		return this;
	}

	off<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void)
	{
		super.off(event, listener as (...args: any[]) => void);

		return this;
	}

	addListener<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void)
	{
		super.on(event, listener as (...args: any[]) => void);

		return this;
	}
	prependListener<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void)
	{
		super.prependListener(event, listener as (...args: any[]) => void);

		return this;
	}

	once<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void) 
	{
		super.once(event, listener as (...args: any[]) => void);

		return this;
	}
	prependOnceListener<K extends keyof E & string>(event: K,
		listener: (...args: E[K]) => void)
	{
		super.prependOnceListener(event, listener as (...args: any[]) => void);
	
		return this;
	}

	removeListener<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void)
	{
		super.off(event, listener as (...args: any[]) => void);

		return this;
	}
}
