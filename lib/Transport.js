const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const utils = require('./utils');
const ortc = require('./ortc');
const Producer = require('./Producer');
const Consumer = require('./Consumer');
const DataProducer = require('./DataProducer');
const DataConsumer = require('./DataConsumer');

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
	 * @emits @newdataproducer
	 * @emits @dataproducerclose
	 */
	constructor(
		{
			internal,
			data,
			channel,
			appData,
			getRouterRtpCapabilities,
			getProducerById,
			getDataProducerById
		})
	{
		super(logger);

		logger.debug('constructor()');

		// Internal data.
		// @type {Object}
		// - .routerId
		// - .transportId
		this._internal = internal;

		// Transport specific data.
		// @type {Object}
		this._data = data;

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

		// Function that gets any DataProducer in the Router.
		// @type {Function: DataProducer}
		this._getDataProducerById = getDataProducerById;

		// Map of Producers indexed by id.
		// @type {Map<String, Producer>}
		this._producers = new Map();

		// Map of Consumers indexed by id.
		// @type {Map<String, Consumer>}
		this._consumers = new Map();

		// Map of DataProducers indexed by id.
		// @type {Map<String, DataProducer>}
		this._dataProducers = new Map();

		// Map of DataConsumers indexed by id.
		// @type {Map<String, DataConsumer>}
		this._dataConsumers = new Map();

		// CNAME of Producers in this Transport. This is for the scenario in which
		// a Producer does not signal any CNAME in its RTP parameters.
		// @type {String}
		this._cnameForProducers = null;

		// Array for holding used and unused SCTP stream ids.
		// @type {Buffer}
		this._sctpStreamIds = null;

		// SCTP stream id value counter.
		// @type {Number}
		this._nextSctpStreamId = 0;

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
	 * @emits {producer: DataProducer} newdataproducer
	 * @emits {consumer: DataConsumer} newdataconsumer
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

		// Close every DataProducer.
		for (const dataProducer of this._dataProducers.values())
		{
			dataProducer.transportClosed();

			// Must tell the Router.
			this.emit('@dataproducerclose', dataProducer);
		}
		this._dataProducers.clear();

		// Close every DataConsumer.
		for (const dataConsumer of this._dataConsumers.values())
		{
			dataConsumer.transportClosed();
		}
		this._dataConsumers.clear();

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

		// Close every DataProducer.
		for (const dataProducer of this._dataProducers.values())
		{
			dataProducer.transportClosed();

			// NOTE: No need to tell the Router since it already knows (it has
			// been closed in fact).
		}
		this._dataProducers.clear();

		// Close every DataConsumer.
		for (const dataConsumer of this._dataConsumers.values())
		{
			dataConsumer.transportClosed();
		}
		this._dataConsumers.clear();

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
	 * @param {Boolean} [paused=false] - Whether the Producer must start paused.
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

		// If missing encodings, add one.
		if (!rtpParameters.encodings || !Array.isArray(rtpParameters.encodings))
			rtpParameters.encodings = [ {} ];

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
	 * @param {Object} [preferredLayers] - Preferred spatial and temporal layers.
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
			preferredLayers,
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
			paused,
			preferredLayers
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
	 * Create a DataProducer.
	 *
	 * @param {String} [id] - DataProducer id (just for PipeTransports).
	 * @param {RTCSctpStreamParameters} sctpStreamParameters - Remote SCTP stream
	 *   parameters.
	 * @param {String} [label='']
	 * @param {String} [protocol='']
	 * @param {Object} [appData={}] - Custom app data.
   *
	 * @async
	 * @returns {DataProducer}
	 */
	async produceData(
		{
			id = undefined,
			sctpStreamParameters,
			label = '',
			protocol = '',
			appData = {}
		} = {}
	)
	{
		logger.debug('produceData()');

		if (id && this._dataProducers.has(id))
			throw new TypeError(`a DataProducer with same id "${id}" already exists`);
		else if (typeof sctpStreamParameters !== 'object')
			throw new TypeError('missing sctpStreamParameters');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		const internal = { ...this._internal, dataProducerId: id || uuidv4() };
		const reqData = { sctpStreamParameters, label, protocol };

		const data =
			await this._channel.request('transport.produceData', internal, reqData);

		const dataProducer = new DataProducer(
			{
				internal,
				data,
				channel : this._channel,
				appData
			});

		this._dataProducers.set(dataProducer.id, dataProducer);
		dataProducer.on('@close', () =>
		{
			this._dataProducers.delete(dataProducer.id);
			this.emit('@dataproducerclose', dataProducer);
		});

		this.emit('@newdataproducer', dataProducer);

		// Emit observer event.
		this._observer.safeEmit('newdataproducer', dataProducer);

		return dataProducer;
	}

	/**
	 * Create a DataConsumer.
	 *
	 * @param {String} dataProducerId
	 * @param {Object} [appData={}] - Custom app data.
   *
	 * @async
	 * @virtual
	 * @returns {DataConsumer}
	 */
	async consumeData(
		{
			dataProducerId,
			appData = {}
		} = {}
	)
	{
		logger.debug('consumeData()');

		if (!dataProducerId || typeof dataProducerId !== 'string')
			throw new TypeError('missing dataProducerId');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		const dataProducer = this._getDataProducerById(dataProducerId);

		if (!dataProducer)
			throw Error(`DataProducer with id "${dataProducerId}" not found`);

		const sctpStreamParameters = utils.clone(dataProducer.sctpStreamParameters);
		const { label, protocol } = dataProducer;

		// This may throw.
		const sctpStreamId = this._getNextSctpStreamId();

		this._sctpStreamIds[sctpStreamId] = true;
		sctpStreamParameters.streamId = sctpStreamId;

		const internal = { ...this._internal, dataConsumerId: uuidv4(), dataProducerId };
		const reqData = { sctpStreamParameters, label, protocol };

		const data =
			await this._channel.request('transport.consumeData', internal, reqData);

		const dataConsumer = new DataConsumer(
			{
				internal,
				data,
				channel : this._channel,
				appData
			});

		this._dataConsumers.set(dataConsumer.id, dataConsumer);
		dataConsumer.on('@close', () =>
		{
			this._dataConsumers.delete(dataConsumer.id);

			this._sctpStreamIds[sctpStreamId] = false;
		});
		dataConsumer.on('@dataproducerclose', () =>
		{
			this._dataConsumers.delete(dataConsumer.id);

			this._sctpStreamIds[sctpStreamId] = false;
		});

		// Emit observer event.
		this._observer.safeEmit('newdataconsumer', dataConsumer);

		return dataConsumer;
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

	_getNextSctpStreamId()
	{
		if (
			!this._data.sctpParameters ||
			typeof this._data.sctpParameters.MIS !== 'number'
		)
		{
			throw new TypeError('missing data.sctpParameters.MIS');
		}

		const numStreams = this._data.sctpParameters.MIS;

		if (!this._sctpStreamIds)
			this._sctpStreamIds = Buffer.alloc(numStreams, false);

		let sctpStreamId;

		for (let idx = 0; idx < this._sctpStreamIds.length; ++idx)
		{
			sctpStreamId = (this._nextSctpStreamId + idx) % this._sctpStreamIds.length;

			if (!this._sctpStreamIds[sctpStreamId])
			{
				this._nextSctpStreamId = sctpStreamId + 1;

				return sctpStreamId;
			}
		}

		throw new Error('no sctpStreamId available');
	}
}

module.exports = Transport;
