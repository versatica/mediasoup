const os = require('os');
const path = require('path');
const check = require('check-types');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');
const utils = require('./utils');
const ortc = require('./ortc');
const Worker = require('./Worker');

const DEFAULT_NUM_WORKERS = Object.keys(os.cpus()).length;
const VALID_WORKER_PARAMETERS =
[
	'logLevel',
	'logTags',
	'rtcIPv4',
	'rtcIPv6',
	'rtcAnnouncedIPv4',
	'rtcAnnouncedIPv6',
	'rtcMinPort',
	'rtcMaxPort',
	'dtlsCertificateFile',
	'dtlsPrivateKeyFile'
];

const logger = new Logger('Server');

class Server extends EnhancedEventEmitter
{
	constructor(options)
	{
		super(logger);

		logger.debug('constructor() [options:%o]', options);

		const serverId = utils.randomString();
		const parameters = [];
		let numWorkers = DEFAULT_NUM_WORKERS;

		// Closed flag.
		this._closed = false;

		// Set of Worker instances.
		this._workers = new Set();

		// Clone options.
		options = utils.cloneObject(options);

		// Update numWorkers (if given).
		if (check.integer(options.numWorkers) && check.positive(options.numWorkers))
			numWorkers = options.numWorkers;

		// Remove numWorkers.
		delete options.numWorkers;

		// Normalize some options.

		if (!check.array(options.logTags))
			options.logTags = [];

		if (options.rtcIPv4 === null || options.rtcIPv4 === undefined)
			delete options.rtcIPv4;

		if (options.rtcIPv6 === null || options.rtcIPv6 === undefined)
			delete options.rtcIPv6;

		if (!options.rtcAnnouncedIPv4 || options.rtcIPv4 === false)
			delete options.rtcAnnouncedIPv4;

		if (!options.rtcAnnouncedIPv6 || options.rtcIPv6 === false)
			delete options.rtcAnnouncedIPv6;

		if (!check.greaterOrEqual(options.rtcMinPort, 1024))
			options.rtcMinPort = 10000;

		if (!check.lessOrEqual(options.rtcMaxPort, 65535))
			options.rtcMaxPort = 59999;

		if (check.nonEmptyString(options.dtlsCertificateFile))
			options.dtlsCertificateFile = path.resolve(options.dtlsCertificateFile);

		if (check.nonEmptyString(options.dtlsPrivateKeyFile))
			options.dtlsPrivateKeyFile = path.resolve(options.dtlsPrivateKeyFile);

		// Remove rtcMinPort/rtcMaxPort (will be added per worker).

		const totalRtcMinPort = options.rtcMinPort;
		const totalRtcMaxPort = options.rtcMaxPort;

		delete options.rtcMinPort;
		delete options.rtcMaxPort;

		for (const key of Object.keys(options))
		{
			if (!check.includes(VALID_WORKER_PARAMETERS, key))
			{
				logger.warn('ignoring unknown option "%s"', key);

				continue;
			}

			switch (key)
			{
				case 'logTags':
					for (const tag of options.logTags)
					{
						parameters.push(`--logTag=${String(tag).trim()}`);
					}
					break;

				default:
					parameters.push(`--${key}=${String(options[key]).trim()}`);
			}
		}

		// Create Worker instances.
		for (let i = 1; i <= numWorkers; i++)
		{
			const workerId = `${serverId}#${i}`;
			const workerParameters = parameters.slice(0);

			// Distribute RTC ports for each worker.

			let rtcMinPort = totalRtcMinPort;
			let rtcMaxPort = totalRtcMaxPort;
			const numPorts = Math.floor((rtcMaxPort - rtcMinPort) / numWorkers);

			rtcMinPort = rtcMinPort + (numPorts * (i - 1));
			rtcMaxPort = rtcMinPort + numPorts;

			if (rtcMinPort % 2 !== 0)
				rtcMinPort++;

			if (rtcMaxPort % 2 === 0)
				rtcMaxPort--;

			workerParameters.push(`--rtcMinPort=${rtcMinPort}`);
			workerParameters.push(`--rtcMaxPort=${rtcMaxPort}`);

			// Create a Worker instance (do it in a separate method to avoid creating
			// a callback function within a loop).
			this._addWorker(new Worker(workerId, workerParameters));
		}
	}

	get closed()
	{
		return this._closed;
	}

	/**
	 * Close the Server.
	 */
	close()
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		this.safeEmit('close');

		// Close every Worker.
		for (const worker of this._workers)
		{
			worker.close();
		}
	}

	/**
	 * Dump the Server.
	 *
	 * @private
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Server closed'));

		const promises = [];

		for (const worker of this._workers)
		{
			promises.push(worker.dump());
		}

		return Promise.all(promises)
			.then((datas) =>
			{
				const json =
				{
					workers : datas
				};

				return json;
			});
	}

	/**
	 * Update Server settings.
	 *
	 * @param {Object} options - Object with modified settings.
	 *
	 * @return {Promise}
	 */
	updateSettings(options)
	{
		logger.debug('updateSettings() [options:%o]', options);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Server closed'));

		const promises = [];

		for (const worker of this._workers)
		{
			promises.push(worker.updateSettings(options));
		}

		return Promise.all(promises);
	}

	/**
	 * Create a Room instance.
	 *
	 * @param {array<RoomMediaCodec>} mediaCodecs
	 *
	 * @return {Room}
	 */
	Room(mediaCodecs)
	{
		logger.debug('Room()');

		if (this._closed)
			throw new errors.InvalidStateError('Server closed');

		// This may throw.
		const rtpCapabilities = ortc.generateRoomRtpCapabilities(mediaCodecs);
		const worker = this._getRandomWorker();
		const room = worker.Room({ rtpCapabilities });

		this.safeEmit('newroom', room);

		return room;
	}

	_addWorker(worker)
	{
		// Store the Worker instance and remove it when closed.
		// Also, if it is the latest Worker then close the Server.
		this._workers.add(worker);
		worker.on('@close', () =>
		{
			this._workers.delete(worker);

			if (this._workers.size === 0 && !this._closed)
			{
				logger.debug('latest Worker closed, closing Server');

				this.close();
			}
		});
	}

	_getRandomWorker()
	{
		const array = Array.from(this._workers);

		return array[array.length * Math.random() << 0];
	}
}

module.exports = Server;
