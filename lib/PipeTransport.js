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
	constructor(params)
	{
		super(params);

		logger.debug('constructor()');

		const { data } = params;

		// PipeTransport data.
		// @type {Object}
		// - .tuple
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
	 * @async
	 * @override
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
		this._observer.safeEmit('newconsumer', consumer);

		return consumer;
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

module.exports = PipeTransport;
