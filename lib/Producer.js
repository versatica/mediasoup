const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');

const logger = new Logger('Producer');

class Producer extends EnhancedEventEmitter
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
		// - .producerId
		this._internal = internal;

		// Producer data.
		// @type {Object}
		// - .kind
		// - .rtpParameters
		// - .consumableRtpParameters
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

		// Paused flag.
		// @type {Boolean}
		this._paused = false;
	}

	/**
	 * Producer id.
	 *
	 * @returns {String}
	 */
	get id()
	{
		return this._internal.producerId;
	}

	/**
	 * Whether the Producer is closed.
	 *
	 * @returns {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * Media kind.
	 *
	 * @returns {String}
	 */
	get kind()
	{
		return this._data.kind;
	}

	/**
	 * RTP parameters.
	 *
	 * @returns {RTCRtpParameters}
	 */
	get rtpParameters()
	{
		return this._data.rtpParameters;
	}

	/**
	 * Consumable RTP parameters.
	 *
	 * @private
	 * @returns {RTCRtpParameters}
	 */
	get consumableRtpParameters()
	{
		return this._data.consumableRtpParameters;
	}

	/**
	 * Whether the Producer is paused.
	 *
	 * @return {Boolean}
	 */
	get paused()
	{
		return this._paused;
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
	 * Close the Producer.
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.producerId);

		this._channel.request('producer.close', this._internal)
			.catch(() => {});

		this.emit('@close');
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
	}

	/**
	 * Dump Producer.
	 *
	 * @async
	 * @returns {Object}
	 */
	async dump()
	{
		logger.debug('dump()');

		return this._channel.request('producer.dump', this._internal);
	}

	/**
	 * Pause the Producer.
	 *
	 * @async
	 */
	async pause()
	{
		if (this._paused)
			return;

		logger.debug('pause()');

		await this._channel.request('producer.pause', this._internal);

		this._paused = true;
	}

	/**
	 * Resume the Producer.
	 *
	 * @async
	 */
	async resume()
	{
		if (!this._paused)
			return;

		logger.debug('resume()');

		await this._channel.request('producer.resume', this._internal);

		this._paused = false;
	}

	/**
	 * Get Producer stats.
	 *
	 * @async
	 * @returns {Array<Object>}
	 */
	async getStats()
	{
		logger.debug('getStats()');

		return this._channel.request('producer.getStats', this._internal);
	}
}

module.exports = Producer;
