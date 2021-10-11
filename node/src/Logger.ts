import debug from 'debug';

const APP_NAME = 'mediasoup';

export class Logger
{
	readonly #debug: debug.Debugger;
	readonly #warn: debug.Debugger;
	readonly #error: debug.Debugger;

	constructor(prefix?: string)
	{
		if (prefix)
		{
			this.#debug = debug(`${APP_NAME}:${prefix}`);
			this.#warn = debug(`${APP_NAME}:WARN:${prefix}`);
			this.#error = debug(`${APP_NAME}:ERROR:${prefix}`);
		}
		else
		{
			this.#debug = debug(APP_NAME);
			this.#warn = debug(`${APP_NAME}:WARN`);
			this.#error = debug(`${APP_NAME}:ERROR`);
		}

		/* eslint-disable no-console */
		this.#debug.log = console.info.bind(console);
		this.#warn.log = console.warn.bind(console);
		this.#error.log = console.error.bind(console);
		/* eslint-enable no-console */
	}

	get debug(): debug.Debugger
	{
		return this.#debug;
	}

	get warn(): debug.Debugger
	{
		return this.#warn;
	}

	get error(): debug.Debugger
	{
		return this.#error;
	}
}
