const Logger = require('./Logger');
const Transport = require('./Transport');

const logger = new Logger('PlainRtpTransport');

class PlainRtpTransport extends Transport
{
	/**
	 * @private
	 *
	 * @emits {sctpState: String} sctpstatechange
	 */
	constructor(params)
	{
		super(params);

		logger.debug('constructor()');

		const { data } = params;

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
		// - .sctpParameters
		//   - .port
		//   - .OS
		//   - .MIS
		//   - .maxMessageSize
		// - .sctpState
		this._data =
		{
			tuple          : data.tuple,
			rtcpTuple      : data.rtcpTuple,
			sctpParameters : data.sctpParameters,
			sctpState      : data.sctpState
		};
	}

	/**
	 * @type {Object}
	 */
	get tuple()
	{
		return this._data.tuple;
	}

	/**
	 * @type {Object}
	 */
	get rtcpTuple()
	{
		return this._data.rtcpTuple;
	}

	/**
	 * @type {Object}
	 */
	get sctpParameters()
	{
		return this._data.sctpParameters;
	}

	/**
	 * @type {String}
	 */
	get sctpState()
	{
		return this._data.sctpState;
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @type {EventEmitter}
	 *
	 * @emits close
	 * @emits {producer: Producer} newproducer
	 * @emits {consumer: Consumer} newconsumer
	 * @emits {producer: DataProducer} newdataproducer
	 * @emits {consumer: DataConsumer} newdataconsumer
	 * @emits {sctpState: String} sctpstatechange
	 */
	get observer()
	{
		return this._observer;
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

		if (this._data.sctpState)
			this._data.sctpState = 'closed';

		super.close();
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

		if (this._data.sctpState)
			this._data.sctpState = 'closed';

		super.routerClosed();
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
		this._channel.on(this._internal.transportId, (event, data) =>
		{
			switch (event)
			{
				case 'sctpstatechange':
				{
					const { sctpState } = data;

					this._data.sctpState = sctpState;

					this.safeEmit('sctpstatechange', sctpState);

					// Emit observer event.
					this._observer.safeEmit('sctpstatechange', sctpState);

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
