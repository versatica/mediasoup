const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const WebRtcTransport = require('./WebRtcTransport');
const PlainRtpTransport = require('./PlainRtpTransport');

const logger = new Logger('Router');

class Router extends EnhancedEventEmitter
{
	/**
	 * @private
	 *
	 * @emits workerclose
	 * @emits @close
	 */
	constructor({ internal, data, channel })
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Internal data.
		// @type {Object}
		// - .routerId
		this._internal = internal;

		// Router data.
		// @type {Object}
		// - .rtpCapabilities
		this._data = data;

		// Channel instance.
		// @type {Channel}
		this._channel = channel;

		// Map of Transports indexed by id.
		// @type {Map<String, Transport>}
		this._transports = new Map();
	}

	/**
	 * Router id.
	 *
	 * @returns {String}
	 */
	get id()
	{
		return this._internal.routerId;
	}

	/**
	 * Whether the Router is closed.
	 *
	 * @returns {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * RTC capabilities of the Router.
	 *
	 * @returns {RTCRtpCapabilities}
	 */
	get rtpCapabilities()
	{
		return this._data.rtpCapabilities;
	}

	/**
	 * Close the Router.
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		this._channel.request('router.close', this._internal)
			.catch(() => {});

		// Close every Transport.
		for (const transport of this._transports.values())
		{
			transport.routerClosed();
		}
		this._transports.clear();

		this.emit('@close');
	}

	/**
	 * Worker was closed.
	 *
	 * @private
	 */
	workerClosed()
	{
		if (this._closed)
			return;

		logger.debug('workerClosed()');

		this._closed = true;

		// Close every Transport.
		for (const transport of this._transports.values())
		{
			transport.routerClosed();
		}
		this._transports.clear();

		this.safeEmit('workerclose');
	}

	/**
	 * Dump Router.
	 *
	 * @private
	 *
	 * @returns {Object}
	 * @throws {InvalidStateError} if closed.
	 * @throws {Error}
	 */
	async dump()
	{
		logger.debug('dump()');

		return this._channel.request('router.dump', this._internal);
	}

	/**
	 * Get Transport by id.
	 *
	 * @param {String} transportId
	 *
	 * @returns {Transport}
	 */
	getTransportById(transportId)
	{
		return this._transports.get(transportId);
	}

	/**
	 * Create a WebRtcTransport.
	 *
	 * @param {Array<String|Object>} ips - Listen IPs in order of preference.
	 *   Each entry can be a IP string or an object with ip and optional
	 *   announcedIp strings.
	 * @param {Boolean} [enableUdp=true] - Enable UDP.
	 * @param {Boolean} [enableTcp=true] - Enable TCP.
	 * @param {Boolean} [preferUdp=false] - Prefer UDP.
	 * @param {Boolean} [preferTcp=false] - Prefer TCP.
	 * @param {Object} [appData] - Custom app data.
   *
	 * @async
	 * @returns {WebRtcTransport}
	 * @throws {InvalidStateError} if closed.
	 * @throws {TypeError} if wrong arguments.
	 * @throws {Error}
	 */
	async createWebRtcTransport(
		{
			ips,
			enableUdp,
			enableTcp,
			preferUdp,
			preferTcp,
			appData
		} = {}
	)
	{
		logger.debug('createWebRtcTransport()');

		if (!Array.isArray(ips))
			throw new TypeError('missing ips');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		ips = ips.map((ip) =>
		{
			if (ip && typeof ip === 'string')
			{
				return { ip };
			}
			else if (typeof ip === 'object' && typeof ip.ip === 'string')
			{
				return {
					ip          : ip.ip,
					announcedIp : ip.announcedIp && typeof ip.announcedIp === 'string'
						? ip.announcedIp
						: undefined
				};
			}
			else
			{
				throw new TypeError('wrong ip');
			}
		});

		appData = appData || {};

		const internal =
		{
			routerId    : this._internal.routerId,
			transportId : uuidv4()
		};

		const data = { ips, enableUdp, enableTcp, preferUdp, preferTcp };

		const result =
			await this._channel.request('router.createWebRtcTransport', internal, data);

		const transport = new WebRtcTransport(
			{
				internal,
				data    : result,
				channel : this._channel,
				appData
			});

		this._transports.set(transport.id, transport);
		transport.on('@close', () => this._transports.delete(transport.id));

		return transport;
	}

	/**
	 * Create a PlainRtpTransport.
	 *
	 * @param {String} ip - Listen IP.
	 * @param {String} [announcedIp] - Announcded IP.
	 * @param {Object} [appData] - Custom app data.
   *
	 * @async
	 * @returns {PlainRtpTransport}
	 * @throws {InvalidStateError} if closed.
	 * @throws {TypeError} if wrong arguments.
	 * @throws {Error}
	 */
	async createPlainRtpTransport({ ip, announcedIp, appData } = {})
	{
		logger.debug('createPlainRtpTransport()');

		if (!ip || typeof ip !== 'string')
			throw new TypeError('missing ip');
		else if (announcedIp && typeof announcedIp !== 'string')
			throw new TypeError('wrong announcedIp');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		appData = appData || {};

		const internal =
		{
			routerId    : this._internal.routerId,
			transportId : uuidv4()
		};

		const data = { ip, announcedIp };

		const result =
			await this._channel.request('router.createPlainRtpTransport', internal, data);

		const transport = new PlainRtpTransport(
			{
				internal,
				data    : result,
				channel : this._channel,
				appData
			});

		this._transports.set(transport.id, transport);
		transport.on('@close', () => this._transports.delete(transport.id));

		return transport;
	}
}

module.exports = Router;
