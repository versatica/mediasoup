const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const ortc = require('./ortc');
const Channel = require('./Channel');
const Router = require('./Router');

const logger = new Logger('Worker');

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
	constructor({ child })
	{
		logger.debug('constructor()');

		super();

		this._workerLogger = new Logger(`worker[pid:${child.pid}]`);

		// Worker process identifier (PID).
		// @type {Number}
		this._pid = child.pid;

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// mediasoup-worker child process.
		// @type {ChildProcess}
		this._child = child;

		// Channel instance.
		// @type {Channel}
		this._channel = new Channel({ socket: child.stdio[3], pid: this._pid });

		// Set of Router instances.
		// @type {Set<Router>}
		this._routers = new Set();

		let spawnDone = false;

		// Listen for 'ready' notification.
		this._channel.once(String(this._pid), (event) =>
		{
			if (!spawnDone && event === 'running')
			{
				spawnDone = true;

				logger.debug('worker process running [pid:%s]', this._pid);

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
						'worker process failed due to wrong settings [pid:%s]', this._pid);

					this.emit('@failure', new TypeError('wrong settings'));
				}
				else
				{
					logger.error(
						'worker process failed unexpectedly [pid:%s, code:%s, signal:%s]',
						this._pid, code, signal);

					this.emit(
						'@failure',
						new Error(`[pid:${this._pid}, code:${code}, signal:${signal}]`));
				}
			}
			else
			{
				logger.error(
					'worker process died unexpectedly [pid:%s, code:%s, signal:%s]',
					this._pid, code, signal);

				this.safeEmit(
					'died',
					new Error(`[pid:${this._pid}, code:${code}, signal:${signal}]`));
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
					'worker process failed [pid:%s]: %s', this._pid, error.message);

				this.emit('@failure', error);
			}
			else
			{
				logger.error(
					'worker process error [pid:%s]: %s', this._pid, error.message);

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
					this._workerLogger.debug(`(stdout) ${line}`);
			}
		});

		// In case of a worker bug, mediasoup will call abort() and log to stderr.
		this._child.stderr.on('data', (buffer) =>
		{
			for (const line of buffer.toString('utf8').split('\n'))
			{
				if (line)
					this._workerLogger.error(`(stderr) ${line}`);
			}
		});
	}

	/**
	 * Worker process identifier (PID).
	 *
	 * @returns {Number}
	 */
	get pid()
	{
		return this._pid;
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
			router.workerClosed();
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
	async createRouter({ mediaCodecs } = {})
	{
		logger.debug('createRouter()');

		// This may throw.
		const rtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

		const internal = { routerId: uuidv4() };
		const data = { rtpCapabilities };

		await this._channel.request('worker.createRouter', internal);

		const router = new Router(
			{
				internal,
				data,
				channel : this._channel
			});

		this._routers.add(router);
		router.on('@close', () => this._routers.delete(router));

		return router;
	}
}

module.exports = Worker;
