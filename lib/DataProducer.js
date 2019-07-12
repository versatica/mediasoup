const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');

const logger = new Logger('DataProducer');

class DataProducer extends EnhancedEventEmitter
{
	/**
	 * @private
	 *
	 * @emits transportclose
	 * @emits @close
	 */
	constructor({ internal, data, channel, appData })
	{
		super(logger);

		logger.debug('constructor()');

		// Internal data.
		// @type {Object}
		// - .routerId
		// - .transportId
		// - .dataProducerId
		this._internal = internal;

		// DataProducer data.
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
	 * DataProducer id.
	 *
	 * @type {String}
	 */
	get id()
	{
		return this._internal.dataProducerId;
	}

	/**
	 * Whether the DataProducer is closed.
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
	 * Close the DataProducer.
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.dataProducerId);

		this._channel.request('dataProducer.close', this._internal)
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

		this.safeEmit('transportclose');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Dump DataProducer.
	 *
	 * @async
	 * @returns {Object}
	 */
	async dump()
	{
		logger.debug('dump()');

		return this._channel.request('dataProducer.dump', this._internal);
	}

	/**
	 * Get DataProducer stats.
	 *
	 * @async
	 * @returns {Array<Object>}
	 */
	async getStats()
	{
		logger.debug('getStats()');

		return this._channel.request('dataProducer.getStats', this._internal);
	}

	/**
	 * @private
	 */
	_handleWorkerNotifications()
	{
		// No need to subscribe to any event.
	}
}

module.exports = DataProducer;
