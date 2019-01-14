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

		// Internal data.
		// @type {Object}
		// - .routerId
		this._internal = internal;

		// Router data.
		// @type {Object}
		// - .rtpCapabilities
		this._data =
		{
			rtpCapabilities : data.rtpCapabilities
		};

		// Channel instance.
		// @type {Channel}
		this._channel = channel;

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Map of Transports indexed by id.
		// @type {Map<String, Transport>}
		this._transports = new Map();

		// Map of Producers indexed by id.
		// @type {Map<String, Producer>}
		this._producers = new Map();
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

		// Clear the Producers map.
		this._producers.clear();

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

		// Clear the Producers map.
		this._producers.clear();

		this.safeEmit('workerclose');
	}

	/**
	 * Dump Router.
	 *
	 * @private
	 *
	 * @returns {Object}
	 */
	async dump()
	{
		logger.debug('dump()');

		return this._channel.request('router.dump', this._internal);
	}

	/**
	 * Create a WebRtcTransport.
	 *
	 * @param {Array<String|Object>} listenIps - Listen IPs in order of preference.
	 *   Each entry can be a IP string or an object with ip and optional
	 *   announcedIp strings.
	 * @param {Boolean} [enableUdp=true] - Enable UDP.
	 * @param {Boolean} [enableTcp=false] - Enable TCP.
	 * @param {Boolean} [preferUdp=false] - Prefer UDP.
	 * @param {Boolean} [preferTcp=false] - Prefer TCP.
	 * @param {Object} [appData={}] - Custom app data.
   *
	 * @async
	 * @returns {WebRtcTransport}
	 */
	async createWebRtcTransport(
		{
			listenIps,
			enableUdp = true,
			enableTcp = false,
			preferUdp = false,
			preferTcp = false,
			appData = {}
		} = {}
	)
	{
		logger.debug('createWebRtcTransport()');

		if (!Array.isArray(listenIps))
			throw new TypeError('missing listenIps');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		listenIps = listenIps.map((listenIp) =>
		{
			if (typeof listenIp === 'string')
			{
				return { ip: listenIp };
			}
			else if (typeof listenIp === 'object')
			{
				return {
					ip          : listenIp.ip,
					announcedIp : listenIp.announcedIp
				};
			}
			else
			{
				throw new TypeError('wrong listenIp');
			}
		});

		const internal = { ...this._internal, transportId: uuidv4() };
		const reqData = { listenIps, enableUdp, enableTcp, preferUdp, preferTcp };

		const data =
			await this._channel.request('router.createWebRtcTransport', internal, reqData);

		const transport = new WebRtcTransport(
			{
				internal,
				data,
				channel                  : this._channel,
				appData,
				getRouterRtpCapabilities : () => this._data.rtpCapabilities,
				getRouterProducerById    : (producerId) => this._producers.get(producerId)
			});

		this._transports.set(transport.id, transport);
		transport.on('@close', () => this._transports.delete(transport.id));
		transport.on('@newproducer', (producer) => this._producers.set(producer.id, producer));
		transport.on('@producerclose', (producer) => this._producers.delete(producer.id));

		return transport;
	}

	/**
	 * Create a PlainRtpTransport.
	 *
	 * @param {String|Object} listenIp - Listen IP string or an object with ip and optional
	 *   announcedIp string.
	 * @param {Boolean} [rtcpMux=true] - Use RTCP-mux.
	 * @param {Object} [appData={}] - Custom app data.
   *
	 * @async
	 * @returns {PlainRtpTransport}
	 */
	async createPlainRtpTransport(
		{
			listenIp,
			rtcpMux = true,
			appData = {}
		} = {}
	)
	{
		logger.debug('createPlainRtpTransport()');

		if (!listenIp)
			throw new TypeError('missing listenIp');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		if (typeof listenIp === 'string')
		{
			listenIp = { ip: listenIp };
		}
		else if (typeof listenIp === 'object')
		{
			listenIp =
			{
				ip          : listenIp.ip,
				announcedIp : listenIp.announcedIp
			};
		}
		else
		{
			throw new TypeError('wrong listenIps');
		}

		const internal = { ...this._internal, transportId: uuidv4() };
		const reqData = { listenIp, rtcpMux };

		const data =
			await this._channel.request('router.createPlainRtpTransport', internal, reqData);

		const transport = new PlainRtpTransport(
			{
				internal,
				data,
				channel               : this._channel,
				appData,
				routerRtpCapabilities : this._data.rtpCapabilities,
				routerProducers       : this._producers
			});

		this._transports.set(transport.id, transport);
		transport.on('@close', () => this._transports.delete(transport.id));
		transport.on('@newproducer', (producer) => this._producers.set(producer.id, producer));
		transport.on('@producerclose', (producer) => this._producers.delete(producer.id));

		return transport;
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
}

module.exports = Router;
