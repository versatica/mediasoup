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
		//   - .local
		//     - .ip
		//     - .port
		//     - .protocol
		//   - .remote
		//     - .ip
		//     - .port
		//     - .protocol
		this._data = data;
	}

	get tuple()
	{
		return this._data.tuple;
	}

	get localIP()
	{
		return this._data.localIP;
	}

	get localPort()
	{
		return this._data.localPort;
	}

	/**
	 * Close the PlainRtpTransport.
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
	 */
	routerClosed()
	{
		if (this._closed)
			return;

		super.routerClosed();

		delete this._data.tuple;
	}

	_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.transportId, (event) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(false);

					break;
				}

				default:
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});
	}
}

module.exports = PlainRtpTransport;
