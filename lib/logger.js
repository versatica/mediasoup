'use strict';

const debug = require('debug');

const APP_NAME = 'mediasoup';

class Logger
{
	constructor(prefix)
	{
		if (prefix)
		{
			this._debug = debug(APP_NAME + ':' + prefix);
			this._warn  = debug(APP_NAME + ':WARN:' + prefix);
			this._error = debug(APP_NAME + ':ERROR:' + prefix);
		}
		else
		{
			this._debug = debug(APP_NAME);
			this._warn  = debug(APP_NAME + ':WARN');
			this._error = debug(APP_NAME + ':ERROR');
		}

		this._warn.log = console.warn.bind(console);
		this._error.log = console.error.bind(console);
	}

	get debug()
	{
		return this._debug;
	}

	get warn()
	{
		return this._warn;
	}

	get error()
	{
		return this._error;
	}
}

module.exports = function(prefix)
{
	return new Logger(prefix);
};
