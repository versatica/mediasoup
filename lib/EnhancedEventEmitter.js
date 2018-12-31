const { EventEmitter } = require('events');
const Logger = require('./Logger');

class EnhancedEventEmitter extends EventEmitter
{
	constructor(logger)
	{
		super();
		this.setMaxListeners(Infinity);

		this._logger = logger || new Logger('EnhancedEventEmitter');
	}

	safeEmit(event, ...args)
	{
		try
		{
			this.emit(event, ...args);
		}
		catch (error)
		{
			this._logger.error(
				'safeEmit() | event listener threw an error [event:%s]:%o',
				event, error);
		}
	}

	safeEmitAsPromise(event, ...args)
	{
		return new Promise((resolve, reject) =>
		{
			const callback = (result) =>
			{
				resolve(result);
			};

			const errback = (error) =>
			{
				this._logger.error(
					'safeEmitAsPromise() | errback called [event:%s]:%o',
					event, error);

				reject(error);
			};

			this.safeEmit(event, ...args, callback, errback);
		});
	}
}

module.exports = EnhancedEventEmitter;
