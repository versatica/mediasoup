const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');

const logger = new Logger('Transport');

class Transport extends EnhancedEventEmitter
{
	/**
	 * @private
	 *
	 * @emits routerclose
	 * @emits @close
	 */
	constructor(internal, channel)
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Internal data.
		// - .routerId
		// - .transportId
		this._internal = internal;

		// Channel instance.
		// @type {Channel}
		this._channel = channel;

		this._handleWorkerNotifications();
	}

	get id()
	{
		return this._internal.transportId;
	}

	get closed()
	{
		return this._closed;
	}

	/**
	 * Close the Transport.
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.transportId);

		this._channel.request('transport.close', this._internal)
			.catch(() => {});

		this.emit('@close');
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 */
	routerClosed()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.transportId);

		this.safeEmit('routerclose');
	}

	/**
	 * Dump Transport.
	 *
	 * @async
	 * @returns {Object}
	 * @throws {InvalidStateError} if closed.
	 * @throws {Error}
	 */
	async dump()
	{
		logger.debug('dump()');

		return this._channel.request('worker.dump', this._internal);
	}

	/**
	 * Get Transport stats.
	 *
	 * @async
	 * @returns {Array{Object}}
	 * @throws {InvalidStateError} if closed.
	 * @throws {Error}
	 */
	async getStats()
	{
		logger.debug('getStats()');

		return this._channel.request('worker.getStats', this._internal);
	}

	/**
	 * @private
	 * @interface
	 */
	_handleWorkerNotifications()
	{
	}
}

module.exports = Transport;
