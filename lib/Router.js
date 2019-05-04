const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const ortc = require('./ortc');
const WebRtcTransport = require('./WebRtcTransport');
const PlainRtpTransport = require('./PlainRtpTransport');
const PipeTransport = require('./PipeTransport');
const AudioLevelObserver = require('./AudioLevelObserver');

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

		// Map of RtpObservers indexed by id.
		// @type {Map<String, RtpObserver>}
		this._rtpObservers = new Map();

		// Map of other Routers and their respective local and remote PipeTransports.
		// @type {Map<Router, Array<PipeTransport>}
		this._mapRouterPipeTransports = new Map();

		// Observer.
		// @type {EventEmitter}
		this._observer = new EnhancedEventEmitter();
	}

	/**
	 * Router id.
	 *
	 * @type {String}
	 */
	get id()
	{
		return this._internal.routerId;
	}

	/**
	 * Whether the Router is closed.
	 *
	 * @type {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * RTC capabilities of the Router.
	 *
	 * @type {RTCRtpCapabilities}
	 */
	get rtpCapabilities()
	{
		return this._data.rtpCapabilities;
	}

	/**
	 * Observer.
	 *
	 * @type {EventEmitter}
	 *
	 * @emits close
	 * @emits {transport: Transport} newtransport
	 */
	get observer()
	{
		return this._observer;
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

		// Close every RtpObserver.
		for (const rtpObserver of this._rtpObservers.values())
		{
			rtpObserver.routerClosed();
		}
		this._rtpObservers.clear();

		// Clear map of Router/PipeTransports.
		this._mapRouterPipeTransports.clear();

		this.emit('@close');

		// Emit observer event.
		this._observer.safeEmit('close');
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

		// Close every RtpObserver.
		for (const rtpObserver of this._rtpObservers.values())
		{
			rtpObserver.routerClosed();
		}
		this._rtpObservers.clear();

		// Clear map of Router/PipeTransports.
		this._mapRouterPipeTransports.clear();

		this.safeEmit('workerclose');

		// Emit observer event.
		this._observer.safeEmit('close');
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
	 * @param {Number} [initialAvailableOutgoingBitrate=600000] - Initial available
	 *   outgoing bitrate (in bps) when the endpoint supports REMB or Transport-CC.
	 * @param {Number} [minimumAvailableOutgoingBitrate=100000] - Minimum available
	 *   outgoing bitrate (in bps).
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
			initialAvailableOutgoingBitrate = 600000,
			minimumAvailableOutgoingBitrate = 100000,
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
					announcedIp : listenIp.announcedIp || undefined
				};
			}
			else
			{
				throw new TypeError('wrong listenIp');
			}
		});

		const internal = { ...this._internal, transportId: uuidv4() };
		const reqData = {
			listenIps,
			enableUdp,
			enableTcp,
			preferUdp,
			preferTcp,
			initialAvailableOutgoingBitrate,
			minimumAvailableOutgoingBitrate
		};

		const data =
			await this._channel.request('router.createWebRtcTransport', internal, reqData);

		const transport = new WebRtcTransport(
			{
				internal,
				data,
				channel                  : this._channel,
				appData,
				getRouterRtpCapabilities : () => this._data.rtpCapabilities,
				getProducerById          : (producerId) => this._producers.get(producerId)
			});

		this._transports.set(transport.id, transport);
		transport.on('@close', () => this._transports.delete(transport.id));
		transport.on('@newproducer', (producer) => this._producers.set(producer.id, producer));
		transport.on('@producerclose', (producer) => this._producers.delete(producer.id));

		// Emit observer event.
		this._observer.safeEmit('newtransport', transport);

		return transport;
	}

	/**
	 * Create a PlainRtpTransport.
	 *
	 * @param {String|Object} listenIp - Listen IP string or an object with ip and
	 *   optional announcedIp string.
	 * @param {Boolean} [rtcpMux=true] - Use RTCP-mux.
	 * @param {Boolean} [comedia=false] - Whether remote IP:port should be
	 *   auto-detected based on first RTP/RTCP packet received. If enabled, connect()
	 *   method must not be called. This option is ignored if multiSource is set.
	 * @param {Boolean} [multiSource=false] - Whether RTP/RTCP from different remote
	 *   IPs:ports is allowed. If set, the transport will just be valid for receiving
	 *   media (consume() cannot be called on it) and connect() must not be called.
	 * @param {Object} [appData={}] - Custom app data.
   *
	 * @async
	 * @returns {PlainRtpTransport}
	 */
	async createPlainRtpTransport(
		{
			listenIp,
			rtcpMux = true,
			comedia = false,
			multiSource = false,
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
				announcedIp : listenIp.announcedIp || undefined
			};
		}
		else
		{
			throw new TypeError('wrong listenIp');
		}

		const internal = { ...this._internal, transportId: uuidv4() };
		const reqData = { listenIp, rtcpMux, comedia, multiSource };

		const data =
			await this._channel.request('router.createPlainRtpTransport', internal, reqData);

		const transport = new PlainRtpTransport(
			{
				internal,
				data,
				channel                  : this._channel,
				appData,
				getRouterRtpCapabilities : () => this._data.rtpCapabilities,
				getProducerById          : (producerId) => this._producers.get(producerId)
			});

		this._transports.set(transport.id, transport);
		transport.on('@close', () => this._transports.delete(transport.id));
		transport.on('@newproducer', (producer) => this._producers.set(producer.id, producer));
		transport.on('@producerclose', (producer) => this._producers.delete(producer.id));

		// Emit observer event.
		this._observer.safeEmit('newtransport', transport);

		return transport;
	}

	/**
	 * Create a PipeTransport.
	 *
	 * @param {String|Object} listenIp - Listen IP string or an object with ip and optional
	 *   announcedIp string.
	 * @param {Object} [appData={}] - Custom app data.
   *
	 * @async
	 * @returns {PipeTransport}
	 */
	async createPipeTransport({ listenIp, appData = {} } = {})
	{
		logger.debug('createPipeTransport()');

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
				announcedIp : listenIp.announcedIp || undefined
			};
		}
		else
		{
			throw new TypeError('wrong listenIp');
		}

		const internal = { ...this._internal, transportId: uuidv4() };
		const reqData = { listenIp };

		const data =
			await this._channel.request('router.createPipeTransport', internal, reqData);

		const transport = new PipeTransport(
			{
				internal,
				data,
				channel                  : this._channel,
				appData,
				getRouterRtpCapabilities : () => this._data.rtpCapabilities,
				getProducerById          : (producerId) => this._producers.get(producerId)
			});

		this._transports.set(transport.id, transport);
		transport.on('@close', () => this._transports.delete(transport.id));
		transport.on('@newproducer', (producer) => this._producers.set(producer.id, producer));
		transport.on('@producerclose', (producer) => this._producers.delete(producer.id));

		// Emit observer event.
		this._observer.safeEmit('newtransport', transport);

		return transport;
	}

	/**
	 * Pipes the given Producer into another Router in same host.
	 *
	 * @param {String} producerId
	 * @param {Router} router
	 * @param {String|Object} [listenIp='127.0.0.1'] - Listen IP string or an
	 *   object with ip and optional announcedIp string.
	 *
	 * @async
	 * @returns {Object} - Contains `pipeConsumer` {Consumer} created in the current
	 *   Router and `pipeProducer` {Producer} created in the destination Router.
	 */
	async pipeToRouter({ producerId, router, listenIp = '127.0.0.1' } = {})
	{
		if (!producerId)
			throw new TypeError('missing producerId');
		else if (!router)
			throw new TypeError('Router not found');
		else if (router === this)
			throw new TypeError('cannot use this Router as destination');

		const producer = this._producers.get(producerId);

		if (!producer)
			throw new TypeError('Producer not found');

		let pipeTransportPair = this._mapRouterPipeTransports.get(router);
		let localPipeTransport;
		let remotePipeTransport;

		if (pipeTransportPair)
		{
			localPipeTransport = pipeTransportPair[0];
			remotePipeTransport = pipeTransportPair[1];
		}
		else
		{
			try
			{
				pipeTransportPair = await Promise.all(
					[
						this.createPipeTransport({ listenIp }),
						router.createPipeTransport({ listenIp })
					]);

				localPipeTransport = pipeTransportPair[0];
				remotePipeTransport = pipeTransportPair[1];

				await Promise.all(
					[
						localPipeTransport.connect(
							{
								ip   : remotePipeTransport.tuple.localIp,
								port : remotePipeTransport.tuple.localPort
							}),
						remotePipeTransport.connect(
							{
								ip   : localPipeTransport.tuple.localIp,
								port : localPipeTransport.tuple.localPort
							})
					]);

				localPipeTransport.observer.on('close', () =>
				{
					remotePipeTransport.close();
					this._mapRouterPipeTransports.delete(router);
				});

				remotePipeTransport.observer.on('close', () =>
				{
					localPipeTransport.close();
					this._mapRouterPipeTransports.delete(router);
				});

				this._mapRouterPipeTransports.set(
					router, [ localPipeTransport, remotePipeTransport ]);
			}
			catch (error)
			{
				logger.error(
					'pipeToRouter() | error creating PipeTransport pair:%o',
					error);

				if (localPipeTransport)
					localPipeTransport.close();

				if (remotePipeTransport)
					remotePipeTransport.close();

				throw error;
			}
		}

		let pipeConsumer;
		let pipeProducer;

		try
		{
			pipeConsumer = await localPipeTransport.consume(
				{
					producerId,
					paused : producer.paused
				});

			pipeProducer = await remotePipeTransport.produce(
				{
					id            : producer.id,
					kind          : pipeConsumer.kind,
					rtpParameters : pipeConsumer.rtpParameters,
					appData       : producer.appData,
					paused        : pipeConsumer.producerPaused
				});

			// Pipe events from the pipe Consumer to the pipe Producer.
			pipeConsumer.observer.on('close', () => pipeProducer.close());
			pipeConsumer.observer.on('pause', () => pipeProducer.pause());
			pipeConsumer.observer.on('resume', () => pipeProducer.resume());

			// Pipe events from the pipe Producer to the pipe Consumer.
			pipeProducer.observer.on('close', () => pipeConsumer.close());

			return { pipeConsumer, pipeProducer };
		}
		catch (error)
		{
			logger.error(
				'pipeToRouter() | error creating pipe Consumer/Producer pair:%o',
				error);

			if (pipeConsumer)
				pipeConsumer.close();

			if (pipeProducer)
				pipeProducer.close();

			throw error;
		}
	}

	/**
	 * Create an AudioLevelObserver.
	 *
	 * @param {Number} [maxEntries=1] - Maximum number of entries in the 'volumes'
	 *                                  event.
	 * @param {Number} [threshold=-80] - Minimum average volume (in dBvo from -127 to 0)
	 *                                   for entries in the 'volumes' event.
	 * @param {Number} [interval=1000] - Interval in ms for checking audio volumes.
	 *
	 * @async
	 * @returns {AudioLevelObserver}
	 */
	async createAudioLevelObserver(
		{
			maxEntries = 1,
			threshold = -80,
			interval = 1000
		} = {}
	)
	{
		logger.debug('createAudioLevelObserver()');

		const internal = { ...this._internal, rtpObserverId: uuidv4() };
		const reqData = { maxEntries, threshold, interval };

		await this._channel.request('router.createAudioLevelObserver', internal, reqData);

		const audioLevelObserver = new AudioLevelObserver(
			{
				internal,
				channel         : this._channel,
				getProducerById : (producerId) => this._producers.get(producerId)
			});

		this._rtpObservers.set(audioLevelObserver.id, audioLevelObserver);
		audioLevelObserver.on('@close', () =>
		{
			this._rtpObservers.delete(audioLevelObserver.id);
		});

		return audioLevelObserver;
	}

	/**
	 * Check whether the given RTP capabilities can consume the given Producer.
	 *
	 * @param {String} producerId
	 * @param {RTCRtpCapabilities} rtpCapabilities - Remote RTP capabilities.
	 *
	 * @returns {Boolean}
	 */
	canConsume({ producerId, rtpCapabilities } = {})
	{
		const producer = this._producers.get(producerId);

		if (!producer)
		{
			logger.error(
				'canConsume() | Producer with id "%s" not found', producerId);

			return false;
		}

		try
		{
			return ortc.canConsume(producer.consumableRtpParameters, rtpCapabilities);
		}
		catch (error)
		{
			logger.error('canConsume() | unexpected error: %s', String(error));

			return false;
		}
	}
}

module.exports = Router;
