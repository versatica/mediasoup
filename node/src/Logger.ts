import debug from 'debug';

const APP_NAME = 'mediasoup';

export class Logger
{
	private readonly _debug: debug.Debugger;
	private readonly _warn: debug.Debugger;
	private readonly _error: debug.Debugger;

	constructor(prefix?: string)
	{
		if (prefix)
		{
			this._debug = debug(`${APP_NAME}:${prefix}`);
			this._warn = debug(`${APP_NAME}:WARN:${prefix}`);
			this._error = debug(`${APP_NAME}:ERROR:${prefix}`);
		}
		else
		{
			this._debug = debug(APP_NAME);
			this._warn = debug(`${APP_NAME}:WARN`);
			this._error = debug(`${APP_NAME}:ERROR`);
		}

		/* eslint-disable no-console */
		this._debug.log = console.info.bind(console);
		this._warn.log = console.warn.bind(console);
		this._error.log = console.error.bind(console);
		/* eslint-enable no-console */
	}

	get debug(): debug.Debugger
	{
		return this._debug;
	}

	get warn(): debug.Debugger
	{
		return this._warn;
	}

	get error(): debug.Debugger
	{
		return this._error;
	}
}
