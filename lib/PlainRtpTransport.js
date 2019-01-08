const Logger = require('./Logger');
const Transport = require('./Transport');

const logger = new Logger('PlainRtpTransport');

class PlainRtpTransport extends Transport
{
	constructor({ ...params, data })
	{
		super(params);

		logger.debug('constructor()');

		// PlainRtpTransport data.
		// @type {Object}
		// - .tuple
		//   - .localIp
		//   - .localPort
		//   - .remoteIp
		//   - .remotePort
		//   - .protocol
		// - .rtcpTuple
		//   - .localIp
		//   - .localPort
		//   - .remoteIp
		//   - .remotePort
		//   - .protocol
		this._data =
		{
			tuple     : data.tuple,
			rtcpTuple : data.rtcpTuple
		};
	}

	/**
	 * @override
	 * @returns {String}
	 */
	get type()
	{
		return 'plainrtp';
	}

	/**
	 * @returns {Object}
	 */
	get tuple()
	{
		return this._data.tuple;
	}

	/**
	 * @returns {Object}
	 */
	get rtcpTuple()
	{
		return this._data.rtcpTuple;
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
	 * @param {String} ip - Remote IP.
	 * @param {Number} port - Remote port.
	 * @param {Number} [rtcpPort] - Remote RTCP port (ignored if rtcpMux was true).
	 *
	 * @async
	 * @override
	 */
	async connect({ ip, port, rtcpPort } = {})
	{
		logger.debug('connect()');

		const reqData = { ip, port, rtcpPort };

		await this._channel.request('transport.connect', this._internal, reqData);

		// TODO: Should we update this._data.tuple / rtcpTuple here? Or better get it
		// from the request response?
	}

	/**
	 * @private
	 * @override
	 */
	_handleWorkerNotifications()
	{
		// No need to subscribe to any event.
	}
}

module.exports = PlainRtpTransport;
