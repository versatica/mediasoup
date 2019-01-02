const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
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
	 */
	constructor(internal, channel, appData)
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Internal data.
		// @type {Object}
		// - .routerId
		// - .transportId
		this._internal = internal;

		// Channel instance.
		// @type {Channel}
		this._channel = channel;

		// App custom data.
		// @type {Object}
		this._appData = appData;

		// Map of Producers indexed by id.
		// @type {Map<String, Producer>}
		this._producers = new Map();

		// Map of Consumers indexed by id.
		// @type {Map<String, Consumer>}
		this._consumers = new Map();

		// This method must be implemented in every subclass.
		this._handleWorkerNotifications();
	}

	/**
	 * Transport id.
	 *
	 * @returns {String}
	 */
	get id()
	{
		return this._internal.transportId;
	}

	/**
	 * Whether the Transport is closed.
	 *
	 * @returns {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * App custom data.
	 *
	 * @returns {Object}
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

		this.emit('@close');
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

		this.safeEmit('routerclose');
	}

	/**
	 * Dump Transport.
	 *
	 * @async
	 * @returns {Object}
	 * @throws {InvalidStateError} if closed.
	 * @throws {Error}
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
	 * @returns {Array{Object}}
	 * @throws {InvalidStateError} if closed.
	 * @throws {Error}
	 */
	async getStats()
	{
		logger.debug('getStats()');

		return this._channel.request('transport.getStats', this._internal);
	}

	/**
	 * Get Producer by id.
	 *
	 * @param {String} producerId
	 *
	 * @returns {Producer}
	 */
	getProducerById(producerId)
	{
		return this._producers.get(producerId);
	}

	/**
	 * Get Consumer by id.
	 *
	 * @param {String} consumerId
	 *
	 * @returns {Consumer}
	 */
	getConsumerById(consumerId)
	{
		return this._consumers.get(consumerId);
	}

	/**
	 * Create a Producer.
	 *
	 * @param {String} kind - 'audio'/'video'.
	 * @param {RTCRtpParameters} rtpParameters.
	 * @param {Object} [appData] - Custom app data.
   *
	 * @async
	 * @returns {Producer}
	 * @throws {InvalidStateError} if closed.
	 * @throws {TypeError} if wrong arguments.
	 * @throws {Error}
	 */
	async produce({ kind, rtpParameters, appData } = {})
	{
		logger.debug('produce()');

		if (![ 'audio', 'video' ].includes(kind))
			throw new TypeError(`invalid kind "${kind}"`);
		else if (typeof rtpParameters !== 'object')
			throw new TypeError('missing rtpParameters');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		appData = appData || {};

		const internal = { ...this._internal, producerId: uuidv4() };
		const data = { kind, rtpParameters };

		await this._channel.request('transport.produce', internal, data);

		const producer = new Producer(
			{
				internal,
				data,
				channel : this._channel,
				appData
			});

		this._producers.set(producer.id, producer);
		producer.on('@close', () => this._producers.delete(producer.id));

		return producer;
	}

	/**
	 * Create a Consumer.
	 *
	 * @param {String} producerId
	 * @param {RTCRtpCapabilities} rtpCapabilities.
	 * @param {Object} [appData] - Custom app data.
   *
	 * @async
	 * @returns {Consumer}
	 * @throws {InvalidStateError} if closed.
	 * @throws {TypeError} if wrong arguments.
	 * @throws {Error}
	 */
	async consume({ producerId, rtpCapabilities, appData } = {})
	{
		logger.debug('consume()');

		if (!producerId || typeof producerId !== 'string')
			throw new TypeError('missing producerId');
		else if (typeof rtpCapabilities !== 'object')
			throw new TypeError('missing rtpCapabilities');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		appData = appData || {};

		const internal = { ...this._internal, producerId, consumerId: uuidv4() };
		// TODO: lot of ORTC stuff here.
		const kind = 'hehe';
		const rtpParameters = rtpCapabilities;
		const data = { kind, rtpParameters };

		await this._channel.request('transport.consume', internal, data);

		const consumer = new Consumer(
			{
				internal,
				data,
				channel : this._channel,
				appData
			});

		this._consumers.set(consumer.id, consumer);
		consumer.on('@close', () => this._consumers.delete(consumer.id));

		return consumer;
	}

	/**
	 * @private
	 * @abstract
	 */
	_handleWorkerNotifications()
	{
	}
}

module.exports = Transport;
