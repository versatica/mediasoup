import { EventEmitter } from 'events';
import { Logger } from './Logger';

export class EnhancedEventEmitter extends EventEmitter
{
	// Logger instance.
	protected readonly _logger: Logger;

	constructor(logger?: Logger)
	{
		super();
		this.setMaxListeners(Infinity);

		this._logger = logger || new Logger('EnhancedEventEmitter');
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
			this._logger.error(
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
