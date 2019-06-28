const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');

const logger = new Logger('DataConsumer');

class DataConsumer extends EnhancedEventEmitter
{
	/**
	 * @private
	 *
	 * @emits transportclose
	 * @emits dataproducerclose
	 * @emits @close
	 * @emits @dataproducerclose
	 */
	constructor({ internal, data, channel, appData })
	{
		super(logger);

		logger.debug('constructor()');

		// Internal data.
		// @type {Object}
		// - .routerId
		// - .transportId
		// - .dataConsumerId
		// - .dataProducerId
		this._internal = internal;

		// DataConsumer data.
		// @type {RTCSctpStreamParameters}
		// - .sctpStreamParameters
		// - .label
		// - .protocol
		this._data = data;

		// Channel instance.
		// @type {Channel}
		this._channel = channel;

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// App data.
		// @type {Object}
		this._appData = appData;

		// Observer.
		// @type {EventEmitter}
		this._observer = new EnhancedEventEmitter();

		this._handleWorkerNotifications();
	}

	/**
	 * DataConsumer id.
	 *
	 * @type {String}
	 */
	get id()
	{
		return this._internal.dataConsumerId;
	}

	/**
	 * Associated DataProducer id.
	 *
	 * @type {String}
	 */
	get dataProducerId()
	{
		return this._internal.dataProducerId;
	}

	/**
	 * Whether the DataConsumer is closed.
	 *
	 * @type {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * SCTP stream parameters.
	 *
	 * @type {RTCSctpStreamParameters}
	 */
	get sctpStreamParameters()
	{
		return this._data.sctpStreamParameters;
	}

	/**
	 * DataChannel label.
	 *
	 * @returns {String}
	 */
	get label()
	{
		return this._data.label;
	}

	/**
	 * DataChannel protocol.
	 *
	 * @returns {String}
	 */
	get protocol()
	{
		return this._data.protocol;
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
	 */
	get observer()
	{
		return this._observer;
	}

	/**
	 * Close the DataConsumer.
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.dataConsumerId);

		this._channel.request('dataConsumer.close', this._internal)
			.catch(() => {});

		this.emit('@close');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	transportClosed()
	{
		if (this._closed)
			return;

		logger.debug('transportClosed()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.dataConsumerId);

		this.safeEmit('transportclose');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Dump DataConsumer.
	 *
	 * @async
	 * @returns {Object}
	 */
	async dump()
	{
		logger.debug('dump()');

		return this._channel.request('dataConsumer.dump', this._internal);
	}

	/**
	 * Get DataConsumer stats.
	 *
	 * @async
	 * @returns {Array<Object>}
	 */
	async getStats()
	{
		logger.debug('getStats()');

		return this._channel.request('dataConsumer.getStats', this._internal);
	}

	/**
	 * @private
	 */
	_handleWorkerNotifications()
	{
		this._channel.on(this._internal.dataConsumerId, (event) =>
		{
			switch (event)
			{
				case 'dataproducerclose':
				{
					if (this._closed)
						break;

					this._closed = true;

					// Remove notification subscriptions.
					this._channel.removeAllListeners(this._internal.dataConsumerId);

					this.emit('@dataproducerclose');
					this.safeEmit('dataproducerclose');

					// Emit observer event.
					this._observer.safeEmit('close');

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

module.exports = DataConsumer;
