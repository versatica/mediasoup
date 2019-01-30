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
	 * @emits score
	 * @emits layerschange
	 * @emits @close
	 * @emits @producerclose
	 */
	constructor({ internal, data, channel, appData, producerPaused })
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

		// Started flag.
		// @type {Boolean}
		this._started = false;

		// Paused flag.
		// @type {Boolean}
		this._paused = false;

		// Producer paused flag.
		// @type {Boolean}
		this._producerPaused = producerPaused;

		// Score with in and out values.
		// @type {Object}
		this._score = { in: 0, out: 0 };

		// Preferred video layers (just for video with simulcast or SVC).
		// @type {Object}
		this._preferredLayers = null;

		// Current video layers (just for video with simulcast or SVC).
		// @type {Object}
		this._currentLayers = null;

		this._handleWorkerNotifications();
	}

	/**
	 * Consumer id.
	 *
	 * @returns {String}
	 */
	get id()
	{
		return this._internal.consumerId;
	}

	/**
	 * Associated Producer id.
	 *
	 * @returns {String}
	 */
	get producerId()
	{
		return this._internal.producerId;
	}

	/**
	 * Whether the Consumer is closed.
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
	 * Whether the Consumer is started.
	 *
	 * @returns {Boolean}
	 */
	get started()
	{
		return this._started;
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
	 * Consumer score with in and out values.
	 *
	 * @returns {Object}
	 */
	get score()
	{
		return this._score;
	}

	/**
	 * Preferred video layers.
	 *
	 *  @returns {Object}
	 */
	get preferredLayers()
	{
		return this._preferredLayers;
	}

	/**
	 * Current video layers.
	 *
	 *  @returns {Object}
	 */
	get currentLayers()
	{
		return this._currentLayers;
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
	 * Start the Consumer so it sends media to the remote endpoint.
	 *
	 * @async
	 */
	async start()
	{
		logger.debug('start()');

		await this._channel.request('consumer.start', this._internal);

		this._started = true;
	}

	/**
	 * Pause the Consumer.
	 *
	 * @async
	 */
	async pause()
	{
		logger.debug('pause()');

		await this._channel.request('consumer.pause', this._internal);

		this._paused = true;
	}

	/**
	 * Resume the Consumer.
	 *
	 * @async
	 */
	async resume()
	{
		logger.debug('resume()');

		await this._channel.request('consumer.resume', this._internal);

		this._paused = false;
	}

	/**
	 * Set preferred video layers.
	 *
	 * @async
	 */
	async setPreferredLayers(layers)
	{
		logger.debug('setPreferredLayers() [layers:%o]', layers);

		if (typeof layers !== 'object')
			throw new TypeError('wrong layers');

		const reqData = layers;

		await this._channel.request(
			'consumer.setPreferredLayers', this._internal, reqData);

		this._preferredLayers = layers;
	}

	/**
	 * Request a key frame to the Producer.
	 *
	 * @async
	 */
	async requestKeyFrame()
	{
		logger.debug('requestKeyFrame()');

		return this._channel.request('consumer.requestKeyFrame', this._internal);
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

					break;
				}

				case 'producerpause':
				{
					if (this._producerPaused)
						break;

					this._producerPaused = true;

					this.safeEmit('producerpause');

					break;
				}

				case 'producerresume':
				{
					if (!this._producerPaused)
						break;

					this._producerPaused = false;

					this.safeEmit('producerresume');

					break;
				}

				case 'score':
				{
					const score = data;

					this._score = score;

					this.safeEmit('score', score);

					break;
				}

				case 'layerschange':
				{
					const layers = data;

					this._currentLayers = layers;

					this.safeEmit('layerschange', layers);

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
