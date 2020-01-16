import { EventEmitter } from 'events';
import { Logger } from './Logger';

const logger = new Logger('EnhancedEventEmitter');

export class EnhancedEventEmitter extends EventEmitter
{
	constructor()
	{
		super();
		this.setMaxListeners(Infinity);
	}

	safeEmit(event: string, ...args: any[]): boolean
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

	async safeEmitAsPromise(event: string, ...args: any[]): Promise<any>
	{
		return new Promise((resolve, reject) => (
			this.safeEmit(event, ...args, resolve, reject)
		));
	}
}
