const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');

const logger = new Logger('Consumer');

class Consumer extends EnhancedEventEmitter
{
	/**
	 * @private
	 *
	 * @emits transportclose
	 * @emits producerclose
	 * @emits producerpause
	 * @emits producerresume
	 * @emits {producer: Number, consumer: Number} score
	 * @emits {spatialLayer: Number|Null} layerschange
	 * @emits @close
	 * @emits @producerclose
	 */
	constructor({ internal, data, channel, appData, paused, producerPaused, score })
	{
		super(logger);

		logger.debug('constructor()');

		// Internal data.
		// @type {Object}
		// - .routerId
		// - .transportId
		// - .consumerId
		// - .producerId
		this._internal = internal;

		// Consumer data.
		// @type {Object}
		// - .kind
		// - .rtpParameters
		// - .type
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

		// Producer paused flag.
		// @type {Boolean}
		this._producerPaused = producerPaused;

		// Score with producer and consumer keys.
		// @type {Object}
		this._score = score;

		// Current video layers (just for video with simulcast or SVC).
		// @type {Object}
		this._currentLayers = null;

		// Observer.
		// @type {EventEmitter}
		this._observer = new EnhancedEventEmitter();

		this._handleWorkerNotifications();
	}

	/**
	 * Consumer id.
	 *
	 * @type {String}
	 */
	get id()
	{
		return this._internal.consumerId;
	}

	/**
	 * Associated Producer id.
	 *
	 * @type {String}
	 */
	get producerId()
	{
		return this._internal.producerId;
	}

	/**
	 * Whether the Consumer is closed.
	 *
	 * @type {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * Media kind.
	 *
	 * @type {String}
	 */
	get kind()
	{
		return this._data.kind;
	}

	/**
	 * RTP parameters.
	 *
	 * @type {RTCRtpParameters}
	 */
	get rtpParameters()
	{
		return this._data.rtpParameters;
	}

	/**
	 * Associated Producer type.
	 *
	 * @type {String} - It can be 'simple', 'simulcast' or 'svc'.
	 */
	get type()
	{
		return this._data.type;
	}

	/**
	 * Whether the Consumer is paused.
	 *
	 * @return {Boolean}
	 */
	get paused()
	{
		return this._paused;
	}

	/**
	 * Whether the associate Producer  is paused.
	 *
	 * @return {Boolean}
	 */
	get producerPaused()
	{
		return this._producerPaused;
	}

	/**
	 * Consumer score with producer and consumer keys.
	 *
	 * @type {Object}
	 */
	get score()
	{
		return this._score;
	}

	/**
	 * Current video layers.
	 *
	 *  @type {Object}
	 */
	get currentLayers()
	{
		return this._currentLayers;
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
	 * @emits pause
	 * @emits resume
	 * @emits {producer: Number, consumer: Number} score
	 * @emits {spatialLayer: Number|Null} layerschange
	 */
	get observer()
	{
		return this._observer;
	}

	/**
	 * Close the Consumer.
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.consumerId);

		this._channel.request('consumer.close', this._internal)
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

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.consumerId);

		this._closed = true;

		this.safeEmit('transportclose');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Dump Consumer.
	 *
	 * @async
	 * @returns {Object}
	 */
	async dump()
	{
		logger.debug('dump()');

		return this._channel.request('consumer.dump', this._internal);
	}

	/**
	 * Get Consumer stats.
	 *
	 * @async
	 * @returns {Array<Object>}
	 */
	async getStats()
	{
		logger.debug('getStats()');

		return this._channel.request('consumer.getStats', this._internal);
	}

	/**
	 * Pause the Consumer.
	 *
	 * @async
	 */
	async pause()
	{
		logger.debug('pause()');

		const wasPaused = this._paused || this._producerPaused;

		await this._channel.request('consumer.pause', this._internal);

		this._paused = true;

		// Emit observer event.
		if (!wasPaused)
			this._observer.safeEmit('pause');
	}

	/**
	 * Resume the Consumer.
	 *
	 * @async
	 */
	async resume()
	{
		logger.debug('resume()');

		const wasPaused = this._paused || this._producerPaused;

		await this._channel.request('consumer.resume', this._internal);

		this._paused = false;

		// Emit observer event.
		if (wasPaused && !this._producerPaused)
			this._observer.safeEmit('resume');
	}

	/**
	 * Set preferred video layers.
	 *
	 * @async
	 */
	async setPreferredLayers({ spatialLayer, temporalLayer } = {})
	{
		logger.debug('setPreferredLayers()');

		const reqData = { spatialLayer, temporalLayer };

		await this._channel.request(
			'consumer.setPreferredLayers', this._internal, reqData);
	}

	/**
	 * Request a key frame to the Producer.
	 *
	 * @async
	 */
	async requestKeyFrame()
	{
		logger.debug('requestKeyFrame()');

		await this._channel.request('consumer.requestKeyFrame', this._internal);
	}

	_handleWorkerNotifications()
	{
		this._channel.on(this._internal.consumerId, (event, data) =>
		{
			switch (event)
			{
				case 'producerclose':
				{
					if (this._closed)
						break;

					this._closed = true;

					// Remove notification subscriptions.
					this._channel.removeAllListeners(this._internal.consumerId);

					this.emit('@producerclose');
					this.safeEmit('producerclose');

					// Emit observer event.
					this._observer.safeEmit('close');

					break;
				}

				case 'producerpause':
				{
					if (this._producerPaused)
						break;

					const wasPaused = this._paused || this._producerPaused;

					this._producerPaused = true;

					this.safeEmit('producerpause');

					// Emit observer event.
					if (!wasPaused)
						this._observer.safeEmit('pause');

					break;
				}

				case 'producerresume':
				{
					if (!this._producerPaused)
						break;

					const wasPaused = this._paused || this._producerPaused;

					this._producerPaused = false;

					this.safeEmit('producerresume');

					// Emit observer event.
					if (wasPaused && !this._paused)
						this._observer.safeEmit('resume');

					break;
				}

				case 'score':
				{
					const score = data;

					this._score = score;

					this.safeEmit('score', score);

					// Emit observer event.
					this._observer.safeEmit('score', score);

					break;
				}

				case 'layerschange':
				{
					const layers = data;

					this._currentLayers = layers;

					this.safeEmit('layerschange', layers);

					// Emit observer event.
					this._observer.safeEmit('layerschange', layers);

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

module.exports = Consumer;
