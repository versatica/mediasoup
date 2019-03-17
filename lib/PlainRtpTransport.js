const Logger = require('./Logger');
const Transport = require('./Transport');

const logger = new Logger('PlainRtpTransport');

class PlainRtpTransport extends Transport
{
	/**
	 * @private
	 */
	constructor({ data, ...params })
	{
		super(params);

		logger.debug('constructor()');

		// PlainRtpTransport data.
		// @type {Object}
		// - .rtcpMux
		// - .comedia
		// - .multiSource
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

		const data =
			await this._channel.request('transport.connect', this._internal, reqData);

		// Update data.
		this._data.tuple = data.tuple;
		this._data.rtcpTuple = data.rtcpTuple;
	}

	/**
	 * Override Transport.consume() method to reject it if multiSource is set.
	 *
	 * @async
	 * @override
	 * @returns {Consumer}
	 */
	async consume({ ...params } = {})
	{
		if (this._data.multiSource)
			throw new Error('cannot call consume() with multiSource set');

		return super.consume({ ...params });
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
