const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const { InvalidStateError } = require('./errors');
const ortc = require('./ortc');
const Channel = require('./Channel');
const Router = require('./Router');

const logger = new Logger('Worker');
const workerLogger = new Logger('worker');

class Worker extends EnhancedEventEmitter
{
	/**
	 * @private
	 *
	 * @emits died
	 * @emits @succeed
	 * @emits @settingserror
	 * @emits @failure
	 */
	constructor({ id, child })
	{
		logger.debug('constructor()');

		super();

		// Id.
		// @type {String}
		this._id = id;

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Create the mediasoup-worker child process.
		// @type {ChildProcess}
		this._child = child;

		// Channel instance.
		// @type {Channel}
		this._channel = new Channel({ socket: child.stdio[3] });

		// Set of Router instances.
		// @type {Set<Router>}
		this._routers = new Set();

		let spawnDone = false;

		// Listen for 'ready' notification.
		this._channel.once(this._id, (event) =>
		{
			if (!spawnDone && event === 'running')
			{
				spawnDone = true;

				logger.debug('worker process running [id:%s]', id);

				this.emit('@success');
			}
		});

		this._child.on('exit', (code, signal) =>
		{
			if (!spawnDone)
			{
				spawnDone = true;

				if (code === 42)
				{
					logger.error(
						'worker process failed due to wrong settings [id:%s]', id);

					this.emit('@failure', new TypeError('wrong settings'));
				}
				else
				{
					logger.error(
						'worker process failed [id:%s, code:%s]', id, code);

					this.emit('@failure', new Error(`code:${code}, signal:${signal}`));
				}
			}
			else if (code)
			{
				logger.error(
					'worker process died [id:%s, code:%s, signal:%s]',
					id, code, signal);

				this.safeEmit('died', new Error(`code:${code}, signal:${signal}`));
			}

			this._child = null;
			this.close();
		});

		this._child.on('error', (error) =>
		{
			if (!spawnDone)
			{
				spawnDone = true;

				logger.error(
					'worker process failed [id:%s]: %s', id, error.message);

				this.emit('@failure', error);
			}
			else
			{
				logger.error(
					'worker process error [id:%s]: %s', id, error.message);

				this.safeEmit('died', error);
			}

			this._child = null;
			this.close();
		});

		// Be ready for 3rd party worker libraries logging to stdout.
		this._child.stdout.on('data', (buffer) =>
		{
			for (const line of buffer.toString('utf8').split('\n'))
			{
				if (line)
					workerLogger.debug(`(stdout) ${line}`);
			}
		});

		// In case of a worker bug, mediasoup will call abort() and log to stderr.
		this._child.stderr.on('data', (buffer) =>
		{
			for (const line of buffer.toString('utf8').split('\n'))
			{
				if (line)
					workerLogger.error(`(stderr) ${line}`);
			}
		});
	}

	/**
	 * Worker id.
	 *
	 * @returns {String}
	 */
	get id()
	{
		return this._id;
	}

	/**
	 * Whether the Worker is closed.
	 *
	 * @returns {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * Close the Worker.
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Kill the worker process.
		if (this._child)
		{
			// Remove event listeners but leave a fake 'error' hander to avoid
			// propagation.
			this._child.removeAllListeners('exit');
			this._child.removeAllListeners('error');
			this._child.on('error', () => {});
			this._child.kill('SIGTERM');
			this._child = null;
		}

		// Close the Channel instance.
		this._channel.close();

		// Close every Router.
		for (const router of this._routers)
		{
			router.routerClosed();
		}
	}

	/**
	 * Dump Worker.
	 *
	 * @async
	 * @returns {Object}
	 * @throws {InvalidStateError} if closed.
	 * @throws {Error}
	 */
	async dump()
	{
		logger.debug('dump()');

		if (this._closed)
			throw new InvalidStateError('closed');

		return this._channel.request('worker.dump');
	}

	/**
	 * Update settings.
	 *
	 * @param {String} [logLevel]
   * @param {Array<String>} [logTags]
   *
	 * @async
	 * @throws {InvalidStateError} if closed.
	 * @throws {Error}
	 */
	async updateSettings({ logLevel, logTags })
	{
		logger.debug('updateSettings()');

		if (this._closed)
			throw new InvalidStateError('closed');

		const data =
		{
			logLevel,
			logTags
		};

		return this._channel.request('worker.updateSettings', null, data);
	}

	/**
	 * Create a Router.
	 *
	 * @param {Array<RTCRtpCodecCapability>} mediaCodecs
   *
	 * @async
	 * @returns {Router}
	 * @throws {InvalidStateError} if closed.
	 * @throws {Error}
	 */
	async createRouter({ mediaCodecs })
	{
		logger.debug('createRouter()');

		if (this._closed)
			throw new InvalidStateError('closed');

		const internal =
		{
			routerId : uuidv4()
		};

		// This may throw.
		const rtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

		await this._channel.request('worker.createRouter', internal, null);

		const router =
			new Router({ internal, rtpCapabilities, channel: this._channel });

		this._routers.add(router);
		router.on('@close', () => this._routers.delete(router));

		return router;
	}
}

module.exports = Worker;
