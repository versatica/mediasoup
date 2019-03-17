const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const ortc = require('./ortc');
const Transport = require('./Transport');
const Consumer = require('./Consumer');

const logger = new Logger('PipeTransport');

class PipeTransport extends Transport
{
	/**
	 * @private
	 */
	constructor({ data, ...params })
	{
		super(params);

		logger.debug('constructor()');

		// PipeTransport data.
		// @type {Object}
		// - .tuple
		//   - .localIp
		//   - .localPort
		//   - .remoteIp
		//   - .remotePort
		//   - .protocol
		this._data =
		{
			tuple : data.tuple
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
	 * Provide the PipeTransport remote parameters.
	 *
	 * @param {String} ip - Remote IP.
	 * @param {Number} port - Remote port.
	 *
	 * @async
	 * @override
	 */
	async connect({ ip, port } = {})
	{
		logger.debug('connect()');

		const reqData = { ip, port };

		const data =
			await this._channel.request('transport.connect', this._internal, reqData);

		// Update data.
		this._data.tuple = data.tuple;
	}

	/**
	 * Create a pipe Consumer.
	 *
	 * @param {String} producerId
	 * @param {Object} [appData={}] - Custom app data.
   *
   * @override
	 * @async
	 * @returns {Consumer}
	 */
	async consume({ producerId, appData = {} } = {})
	{
		logger.debug('consume()');

		if (!producerId || typeof producerId !== 'string')
			throw new TypeError('missing producerId');
		else if (appData && typeof appData !== 'object')
			throw new TypeError('if given, appData must be an object');

		const producer = this._getProducerById(producerId);

		if (!producer)
			throw Error(`Producer with id "${producerId}" not found`);

		// This may throw.
		const rtpParameters =
			ortc.getPipeConsumerRtpParameters(producer.consumableRtpParameters);

		const internal = { ...this._internal, consumerId: uuidv4(), producerId };
		const reqData =
		{
			kind                   : producer.kind,
			rtpParameters,
			type                   : 'pipe',
			consumableRtpEncodings : producer.consumableRtpParameters.encodings
		};

		const status =
			await this._channel.request('transport.consume', internal, reqData);

		const data = { kind: producer.kind, rtpParameters, type: 'pipe' };

		const consumer = new Consumer(
			{
				internal,
				data,
				channel        : this._channel,
				appData,
				paused         : status.paused,
				producerPaused : status.producerPaused
			});

		this._consumers.set(consumer.id, consumer);
		consumer.on('@close', () => this._consumers.delete(consumer.id));
		consumer.on('@producerclose', () => this._consumers.delete(consumer.id));

		// Emit observer event.
		this.safeEmit('observer:newconsumer', consumer);

		return consumer;
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

module.exports = PipeTransport;
