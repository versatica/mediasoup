const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');

const logger = new Logger('RtpObserver');

class RtpObserver extends EnhancedEventEmitter
{
	/**
	 * @private
	 * @interface
	 *
	 * @emits routerclose
	 * @emits @close
	 */
	constructor({ internal, channel, getProducerById })
	{
		super(logger);

		logger.debug('constructor()');

		// Internal data.
		// @type {Object}
		// - .routerId
		// - .rtpObserverId
		this._internal = internal;

		// Channel instance.
		// @type {Channel}
		this._channel = channel;

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Paused flag.
		// @type {Boolean}
		this._paused = false;

		// Function that gets any Producer in the Router.
		// @type {Function: Producer}
		this._getProducerById = getProducerById;

		// This method must be implemented in every subclass.
		this._handleWorkerNotifications();
	}

	/**
	 * RtpObserver id.
	 *
	 * @type {String}
	 */
	get id()
	{
		return this._internal.rtpObserverId;
	}

	/**
	 * Whether the RtpObserver is closed.
	 *
	 * @type {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * Whether the RtpObserver is paused.
	 *
	 * @type {Boolean}
	 */
	get paused()
	{
		return this._paused;
	}

	/**
	 * Close the RtpObserver.
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
		this._channel.removeAllListeners(this._internal.rtpObserverId);

		this._channel.request('rtpObserver.close', this._internal)
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
		this._channel.removeAllListeners(this._internal.rtpObserverId);

		this.safeEmit('routerclose');
	}

	/**
	 * Pause the RtpObserver.
	 *
	 * @async
	 */
	async pause()
	{
		if (this._paused)
			return;

		logger.debug('pause()');

		await this._channel.request('rtpObserver.pause', this._internal);

		this._paused = true;
	}

	/**
	 * Resume the RtpObserver.
	 *
	 * @async
	 */
	async resume()
	{
		if (!this._paused)
			return;

		logger.debug('resume()');

		await this._channel.request('rtpObserver.resume', this._internal);

		this._paused = false;
	}

	/**
	 * Add a Producer to the RtpObserver.
	 *
	 * @param {String} producerId - The id of a Producer.
	 *
	 * @async
	 */
	async addProducer({ producerId } = {})
	{
		logger.debug('addProducer()');

		const internal = { ...this._internal, producerId };

		await this._channel.request('rtpObserver.addProducer', internal);
	}

	/**
	 * Remove a Producer from the RtpObserver.
	 *
	 * @param {String} producerId - The id of a Producer.
	 *
	 * @async
	 */
	async removeProducer({ producerId } = {})
	{
		logger.debug('removeProducer()');

		const internal = { ...this._internal, producerId };

		await this._channel.request('rtpObserver.removeProducer', internal);
	}

	/**
	 * @private
	 * @abstract
	 */
	_handleWorkerNotifications()
	{
		// Should not happen.
		throw new Error('method not implemented in the subclass');
	}
}

module.exports = RtpObserver;
