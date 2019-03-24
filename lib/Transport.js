const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const ortc = require('./ortc');
const Producer = require('./Producer');
const Consumer = require('./Consumer');

const logger = new Logger('Transport');

class Transport extends EnhancedEventEmitter
{
	/**
	 * @private
	 * @interface
	 *
	 * @emits routerclose
	 * @emits @close
	 * @emits @newproducer
	 * @emits @producerclose
	 */
	constructor(
		{
			internal,
			channel,
			appData,
			getRouterRtpCapabilities,
			getProducerById
		})
	{
		super(logger);

		logger.debug('constructor()');

		// Internal data.
		// @type {Object}
		// - .routerId
		// - .transportId
		this._internal = internal;

		// Channel instance.
		// @type {Channel}
		this._channel = channel;

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// App custom data.
		// @type {Object}
		this._appData = appData;

		// Function that returns router RTP capabilities.
		// @type {Function: RTCRtpCapabilities}
		this._getRouterRtpCapabilities = getRouterRtpCapabilities;

		// Function that gets any Producer in the Router.
		// @type {Function: Producer}
		this._getProducerById = getProducerById;

		// Map of Producers indexed by id.
		// @type {Map<String, Producer>}
		this._producers = new Map();

		// Map of Consumers indexed by id.
		// @type {Map<String, Consumer>}
		this._consumers = new Map();

		// CNAME of Producers in this Transport. This is for the scenario in which
		// a Producer does not signal any CNAME in its RTP parameters.
		// @type {String}
		this._cnameForProducers = null;

		// Observer.
		// @type {EventEmitter}
		this._observer = new EnhancedEventEmitter();

		// This method must be implemented in every subclass.
		this._handleWorkerNotifications();
	}

	/**
	 * Transport id.
	 *
	 * @type {String}
	 */
	get id()
	{
		return this._internal.transportId;
	}

	/**
	 * Whether the Transport is closed.
	 *
	 * @type {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * App custom data.
	 *
	 * @type {Object}
	 */
	get appData()
	{
		return this._appData;
	}

	/**
	 * Invalid setter.
	 */
	set appData(appData) // eslint-disable-line no-unused-vars
	{
		throw new Error('cannot override appData object');
	}

	/**
	 * Observer.
	 *
	 * @type {EventEmitter}
	 *
	 * @emits close
	 * @emits {producer: Producer} newproducer
	 * @emits {consumer: Consumer} newconsumer
	 */
	get observer()
	{
		return this._observer;
	}

	/**
	 * Close the Transport.
	 *
	 * @virtual
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.transportId);

		this._channel.request('transport.close', this._internal)
			.catch(() => {});

		// Close every Producer.
		for (const producer of this._producers.values())
		{
			producer.transportClosed();

			// Must tell the Router.
			this.emit('@producerclose', producer);
		}
		this._producers.clear();

		// Close every Consumer.
		for (const consumer of this._consumers.values())
		{
			consumer.transportClosed();
		}
		this._consumers.clear();

		this.emit('@close');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @virtual
	 */
	routerClosed()
	{
		if (this._closed)
			return;

		logger.debug('routerClosed()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.transportId);

		// Close every Producer.
		for (const producer of this._producers.values())
		{
			producer.transportClosed();

			// NOTE: No need to tell the Router since it already knows (it has
			// been closed in fact).
		}
		this._producers.clear();

		// Close every Consumer.
		for (const consumer of this._consumers.values())
		{
			consumer.transportClosed();
		}
		this._consumers.clear();

		this.safeEmit('routerclose');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Dump Transport.
	 *
	 * @async
	 * @returns {Object}
	 */
	async dump()
	{
		logger.debug('dump()');

		return this._channel.request('transport.dump', this._internal);
	}

	/**
	 * Get Transport stats.
	 *
	 * @async
	 * @returns {Array<Object>}
	 */
	async getStats()
	{
		logger.debug('getStats()');

		return this._channel.request('transport.getStats', this._internal);
	}

	/**
	 * Provide the Transport remote parameters.
	 *
	 * @async
	 * @abstract
	 */
	async connect()
	{
		// Should not happen.
		throw new Error('method not implemented in the subclass');
	}

	/**
	 * Create a Producer.
	 *
	 * @param {String} [id] - Producer id (just for PipeTransports).
	 * @param {String} kind - 'audio'/'video'.
	 * @param {RTCRtpParameters} rtpParameters - Remote RTP parameters.
	 * @param {Boolean} [paused=false] - Whether the Consumer must start paused.
	 * @param {Object} [appData={}] - Custom app data.
   *
	 * @async
	 * @returns {Producer}
	 */
	async produce(
		{
			id = undefined,
			kind,
			rtpParameters,
			paused = false,
			appData = {}
		} = {}
	)
	{
		logger.debug('produce()');

		if (id && this._producers.has(id))
			throw new TypeError(`a Producer with same id "${id}" already exists`);
		else if (![ 'audio', 'video' ].includes(kind))
			throw new TypeError(`invalid kind "${kind}"`);
		else if (typeof rtpParameters !== 'object')
			throw new TypeError('missing rtpParameters');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		// Don't do this in PipeTransports since there we must keep CNAME value in
		// each Producer.
		if (this.constructor.name !== 'PipeTransport')
		{
			// If CNAME is given and we don't have yet a CNAME for Producers in this
			// Transport, take it.
			if (!this._cnameForProducers && rtpParameters.rtcp && rtpParameters.rtcp.cname)
			{
				this._cnameForProducers = rtpParameters.rtcp.cname;
			}
			// Otherwise if we don't have yet a CNAME for Producers and the RTP parameters
			// do not include CNAME, create a random one.
			else if (!this._cnameForProducers)
			{
				this._cnameForProducers = uuidv4().substr(0, 8);
			}

			// Override Producer's CNAME.
			rtpParameters.rtcp = rtpParameters.rtcp || {};
			rtpParameters.rtcp.cname = this._cnameForProducers;
		}

		const routerRtpCapabilities = this._getRouterRtpCapabilities();

		// This may throw.
		const rtpMapping = ortc.getProducerRtpParametersMapping(
			rtpParameters, routerRtpCapabilities);

		// This may throw.
		const consumableRtpParameters = ortc.getConsumableRtpParameters(
			kind, rtpParameters, routerRtpCapabilities, rtpMapping);

		const internal = { ...this._internal, producerId: id || uuidv4() };
		const reqData = { kind, rtpParameters, rtpMapping, paused };

		const status =
			await this._channel.request('transport.produce', internal, reqData);

		const data =
		{
			kind,
			rtpParameters,
			type : status.type,
			consumableRtpParameters
		};

		const producer = new Producer(
			{
				internal,
				data,
				channel : this._channel,
				appData,
				paused
			});

		this._producers.set(producer.id, producer);
		producer.on('@close', () =>
		{
			this._producers.delete(producer.id);
			this.emit('@producerclose', producer);
		});

		this.emit('@newproducer', producer);

		// Emit observer event.
		this._observer.safeEmit('newproducer', producer);

		return producer;
	}

	/**
	 * Create a Consumer.
	 *
	 * @param {String} producerId
	 * @param {RTCRtpCapabilities} rtpCapabilities - Remote RTP capabilities.
	 * @param {Boolean} [paused=false] - Whether the Consumer must start paused.
	 * @param {Object} [appData={}] - Custom app data.
   *
	 * @async
	 * @virtual
	 * @returns {Consumer}
	 */
	async consume(
		{
			producerId,
			rtpCapabilities,
			paused = false,
			appData = {}
		} = {}
	)
	{
		logger.debug('consume()');

		if (!producerId || typeof producerId !== 'string')
			throw new TypeError('missing producerId');
		else if (typeof rtpCapabilities !== 'object')
			throw new TypeError('missing rtpCapabilities');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		const producer = this._getProducerById(producerId);

		if (!producer)
			throw Error(`Producer with id "${producerId}" not found`);

		// This may throw.
		const rtpParameters = ortc.getConsumerRtpParameters(
			producer.consumableRtpParameters, rtpCapabilities);

		const internal = { ...this._internal, consumerId: uuidv4(), producerId };
		const reqData =
		{
			kind                   : producer.kind,
			rtpParameters,
			type                   : producer.type,
			consumableRtpEncodings : producer.consumableRtpParameters.encodings,
			paused
		};

		const status =
			await this._channel.request('transport.consume', internal, reqData);

		const data = { kind: producer.kind, rtpParameters, type: producer.type };

		const consumer = new Consumer(
			{
				internal,
				data,
				channel        : this._channel,
				appData,
				paused         : status.paused,
				producerPaused : status.producerPaused,
				score          : status.score
			});

		this._consumers.set(consumer.id, consumer);
		consumer.on('@close', () => this._consumers.delete(consumer.id));
		consumer.on('@producerclose', () => this._consumers.delete(consumer.id));

		// Emit observer event.
		this._observer.safeEmit('newconsumer', consumer);

		return consumer;
	}

	/**
	 * @private
	 * @abstract
	 */
	_handleWorkerNotifications()
	{
		// Should not happen.
		throw new Error('method not implemented in the subclass');
	}
}

module.exports = Transport;
