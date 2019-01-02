const Logger = require('./Logger');
const Transport = require('./Transport');

const logger = new Logger('PlainRtpTransport');

class PlainRtpTransport extends Transport
{
	constructor({ internal, data, channel, appData })
	{
		super({ internal, channel, appData });

		logger.debug('constructor()');

		// TODO
		// PlainRtpTransport data provided by the worker.
		// - .tuple
		//   - .localIp
		//   - .remoteIp
		//   - .localPort
		//   - .remotePort
		//   - .protocol
		this._data = data;
	}

	get tuple()
	{
		return this._data.tuple;
	}

	get localIp()
	{
		return this._data.localIp;
	}

	get localPort()
	{
		return this._data.localPort;
	}

	/**
	 * Close the PlainRtpTransport.
	 *
	 * @override
	 */
	close()
	{
		if (this._closed)
			return;

		super.close();

		delete this._data.tuple;
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @override
	 */
	routerClosed()
	{
		if (this._closed)
			return;

		super.routerClosed();

		delete this._data.tuple;
	}

	/**
	 * Provide the PlainRtpTransport remote parameters.
	 *
	 * @param {Object} transportRemoteParameters - Remote parameters.
	 *
	 * @async
	 * @override
	 */
	async connect({ transportRemoteParameters })
	{
		logger.debug('connect()');

		// TODO: NO.
		const data = { transportRemoteParameters };

		await this._channel.request('transport.connect', this._internal, data);
	}

	/**
	 * @private
	 * @override
	 */
	_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.transportId, (event) =>
		{
			switch (event)
			{
				default:
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});
	}
}

module.exports = PlainRtpTransport;
