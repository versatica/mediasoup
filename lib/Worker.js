const process = require('process');
const path = require('path');
const { spawn } = require('child_process');
const uuidv4 = require('uuid/v4');
const { version } = require('../package.json');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const ortc = require('./ortc');
const Channel = require('./Channel');
const Router = require('./Router');

// If env MEDIASOUP_WORKER_BIN is given, use it as worker binary.
// Otherwise if env MEDIASOUP_BUILDTYPE is 'Debug' use the Debug binary.
// Otherwise use the Release binary.
const workerBin = process.env.MEDIASOUP_WORKER_BIN
	? process.env.MEDIASOUP_WORKER_BIN
	: process.env.MEDIASOUP_BUILDTYPE === 'Debug'
		? path.join(__dirname, '..', 'worker', 'out', 'Debug', 'mediasoup-worker')
		: path.join(__dirname, '..', 'worker', 'out', 'Release', 'mediasoup-worker');

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
	constructor(
		{
			logLevel,
			logTags,
			rtcMinPort,
			rtcMaxPort,
			dtlsCertificateFile,
			dtlsPrivateKeyFile
		})
	{
		logger.debug('constructor()');

		super();

		const workerArgs = [];

		if (typeof logLevel === 'string' && logLevel)
			workerArgs.push(`--logLevel=${logLevel}`);

		for (const logTag of (Array.isArray(logTags) ? logTags : []))
		{
			if (typeof logTag === 'string' && logTag)
				workerArgs.push(`--logTag=${logTag}`);
		}

		if (typeof rtcMinPort === 'number')
			workerArgs.push(`--rtcMinPort=${rtcMinPort}`);

		if (typeof rtcMaxPort === 'number')
			workerArgs.push(`--rtcMaxPort=${rtcMaxPort}`);

		if (typeof dtlsCertificateFile === 'string' && dtlsCertificateFile)
			workerArgs.push(`--dtlsCertificateFile=${dtlsCertificateFile}`);

		if (typeof dtlsPrivateKeyFile === 'string' && dtlsPrivateKeyFile)
			workerArgs.push(`--dtlsPrivateKeyFile=${dtlsPrivateKeyFile}`);

		logger.debug(
			'spawning worker process: %s %s', workerBin, workerArgs.join(' '));

		// mediasoup-worker child process.
		// @type {ChildProcess}
		this._child = spawn(
			// command
			workerBin,
			// args
			workerArgs,
			// options
			{
				env :
				{
					MEDIASOUP_VERSION : version
				},

				detached : false,

				/*
				 * fd 0 (stdin)   : Just ignore it.
				 * fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
				 * fd 2 (stderr)  : Same as stdout.
				 * fd 3 (channel) : Channel fd.
				 */
				stdio : [ 'ignore', 'pipe', 'pipe', 'pipe' ]
			});

		this._workerLogger = new Logger(`worker[pid:${this._child.pid}]`);

		// Worker process identifier (PID).
		// @type {Number}
		this._pid = this._child.pid;

		// Channel instance.
		// @type {Channel}
		this._channel = new Channel(
			{
				socket : this._child.stdio[3],
				pid    : this._pid
			});

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Set of Router instances.
		// @type {Set<Router>}
		this._routers = new Set();

		// Observer.
		// @type {EventEmitter}
		this._observer = new EnhancedEventEmitter();

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
			this._child = null;
			this.close();

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
		});

		this._child.on('error', (error) =>
		{
			this._child = null;
			this.close();

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

		// In case of a worker bug, mediasoup will log to stderr.
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
	 * @type {Number}
	 */
	get pid()
	{
		return this._pid;
	}

	/**
	 * Whether the Worker is closed.
	 *
	 * @type {Boolean}
	 */
	get closed()
	{
		return this._closed;
	}

	/**
	 * Observer.
	 *
	 * @type {EventEmitter}
	 *
	 * @emits close
	 * @emits {router: Router} newrouter
	 */
	get observer()
	{
		return this._observer;
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
		this._routers.clear();

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Dump Worker.
	 *
	 * @async
	 * @returns {Object}
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
	 */
	async updateSettings({ logLevel, logTags } = {})
	{
		logger.debug('updateSettings()');

		const reqData = { logLevel, logTags };

		return this._channel.request('worker.updateSettings', null, reqData);
	}

	/**
	 * Create a Router.
	 *
	 * @param {Array<RTCRtpCodecCapability>} mediaCodecs
   *
	 * @async
	 * @returns {Router}
	 */
	async createRouter({ mediaCodecs } = {})
	{
		logger.debug('createRouter()');

		// This may throw.
		const rtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

		const internal = { routerId: uuidv4() };

		await this._channel.request('worker.createRouter', internal);

		const data = { rtpCapabilities };
		const router = new Router(
			{
				internal,
				data,
				channel : this._channel
			});

		this._routers.add(router);
		router.on('@close', () => this._routers.delete(router));

		// Emit observer event.
		this._observer.safeEmit('newrouter', router);

		return router;
	}
}

module.exports = Worker;
