'use strict';

const EventEmitter = require('events').EventEmitter;
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
}

module.exports = EnhancedEventEmitter;
