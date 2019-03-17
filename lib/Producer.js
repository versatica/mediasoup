const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');

const logger = new Logger('Producer');

class Producer extends EnhancedEventEmitter
{
	/**
	 * @private
	 *
	 * @emits transportclose
	 * @emits {Array<Object>} score
	 * @emits {Object} videoorientationchange
	 * @emits observer:close
	 * @emits observer:pause
	 * @emits observer:resume
	 * @emits {Array<Object>} observer:score
	 * @emits {Object} observer:videoorientationchange
	 * @emits @close
	 */
	constructor({ internal, data, channel, appData, paused })
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
		// - .type
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
		this._paused = paused;

		// Score list
		// @type {Array<Object>}
		this._score = [];

		this._handleWorkerNotifications();
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
	 * Producer type.
	 *
	 * @returns {String} - It can be 'simple', 'simulcast' or 'svc'.
	 */
	get type()
	{
		return this._data.type;
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
	 * Producer score list.
	 *
	 * @returns {Array<Object>}
	 */
	get score()
	{
		return this._score;
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

		// Emit observer event.
		this.safeEmit('observer:close');
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
		this.safeEmit('observer:close');
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

	/**
	 * Pause the Producer.
	 *
	 * @async
	 */
	async pause()
	{
		logger.debug('pause()');

		const wasPaused = this._paused;

		await this._channel.request('producer.pause', this._internal);

		this._paused = true;

		// Emit observer event.
		if (!wasPaused)
			this.safeEmit('observer:pause');
	}

	/**
	 * Resume the Producer.
	 *
	 * @async
	 */
	async resume()
	{
		logger.debug('resume()');

		const wasPaused = this._paused;

		await this._channel.request('producer.resume', this._internal);

		this._paused = false;

		// Emit observer event.
		if (wasPaused)
			this.safeEmit('observer:resume');
	}

	_handleWorkerNotifications()
	{
		this._channel.on(this._internal.producerId, (event, data) =>
		{
			switch (event)
			{
				case 'score':
				{
					const score = data;

					this._score = score;

					this.safeEmit('score', score);

					// Emit observer event.
					this.safeEmit('observer:score', score);

					break;
				}

				case 'videoorientationchange':
				{
					const videoOrientation = data;

					this.safeEmit('videoorientationchange', videoOrientation);

					// Emit observer event.
					this.safeEmit('observer:videoorientationchange', videoOrientation);

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

module.exports = Producer;
