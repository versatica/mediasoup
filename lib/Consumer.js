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
	 * @emits @close
	 * @emits @producerclose
	 */
	constructor({ internal, data, channel, appData })
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

		// Started flag.
		// @type {Boolean}
		this._started = false;

		// App data.
		// @type {Object}
		this._appData = appData;

		// Paused flag.
		// @type {Boolean}
		this._paused = false;

		// Producer paused flag.
		// @type {Boolean}
		this._producerPaused = false;

		this._handleWorkerNotifications();
	}

	/**
	 * Consumer id.
	 *
	 * @returns {String}
	 */
	get id()
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
	 * Whether the Consumer is started.
	 *
	 * @returns {Boolean}
	 */
	get started()
	{
		return this._started;
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
	 * Start the Consumer so it sends media to the remote endpoint.
	 *
	 * @async
	 */
	async start()
	{
		if (this._started)
			return;

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
		if (this._paused)
			return;

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
		if (!this._paused)
			return;

		logger.debug('resume()');

		await this._channel.request('consumer.resume', this._internal);

		this._paused = false;
	}

	/**
	 * Get Consumer stats.
	 *
	 * @async
	 * @returns {Array{Object}}
	 */
	async getStats()
	{
		logger.debug('getStats()');

		return this._channel.request('consumer.getStats', this._internal);
	}

	/**
	 * Request a key frame to the Producer.
	 *
	 * @async
	 */
	async requestKeyFrame()
	{
		logger.debug('requestKeyFrame()');

		if (
			this._closed ||
			!this._started ||
			this._paused ||
			this._producerPaused ||
			this._data.kind !== 'video'
		)
		{
			return;
		}

		return this._channel.request('consumer.requestKeyFrame', this._internal);
	}

	_handleWorkerNotifications()
	{
		// TODO
		// this._channel.on(this._internal.consumerId, (event, data) =>
		this._channel.on(this._internal.consumerId, (event) =>
		{
			switch (event)
			{
				case 'producerclose':
				{
					if (this._closed)
						break;

					this._closed = true;

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

				default:
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});
	}
}

module.exports = Consumer;
