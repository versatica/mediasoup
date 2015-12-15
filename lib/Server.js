'use strict';

const os = require('os');
const path = require('path');
const EventEmitter = require('events').EventEmitter;
const check = require('check-types');
const randomString = require('random-string');
const logger = require('./logger')('Server');
const Worker = require('./Worker');

const DEFAULT_NUM_WORKERS = Object.keys(os.cpus()).length;
const VALID_PARAMETERS =
[
	'logLevel', 'rtcListenIPv4', 'rtcListenIPv6',	'rtcMinPort', 'rtcMaxPort',
	'dtlsCertificateFile', 'dtlsPrivateKeyFile'
];

class Server extends EventEmitter
{
	constructor(options)
	{
		super();

		logger.debug('constructor() [options:%o]', options);

		let serverId = randomString({ numeric: false, length: 6 }).toLowerCase();
		let numWorkers = DEFAULT_NUM_WORKERS;
		let parameters = [];

		options = options || {};

		// Set of Worker instances
		this._workers = new Set();

		if (check.integer(options.numWorkers) && check.positive(options.numWorkers))
		{
			numWorkers = options.numWorkers;
		}

		if (check.nonEmptyString(options.dtlsCertificateFile))
		{
			options.dtlsCertificateFile = path.resolve(options.dtlsCertificateFile);
		}

		if (check.nonEmptyString(options.dtlsPrivateKeyFile))
		{
			options.dtlsPrivateKeyFile = path.resolve(options.dtlsPrivateKeyFile);
		}

		for (let key of Object.keys(options))
		{
			if (check.includes(VALID_PARAMETERS, key))
			{
				parameters.push(`--${key}=${String(options[key])}`);
			}
		}

		logger.debug('constructor() [worker parameters:"%s"]', parameters.join(' '));

		// Create Worker instances
		for (let i = 1; i <= numWorkers; i++)
		{
			let worker;
			let workerId = serverId + '#' + i;

			worker = new Worker(workerId, parameters);

			// Store the Worker instance and remove it when closed
			this._workers.add(worker);
			worker.once('close', () => this._workers.delete(worker));
		}
	}

	close()
	{
		logger.debug('close()');

		// Close every Worker
		this._workers.forEach(worker => worker.close());

		this.emit('close');
	}

	createRoom(options)
	{
		logger.debug('createRoom()');

		let worker = this._getRandomWorker();

		return worker.createRoom(options);
	}

	_getRandomWorker()
	{
		let array = Array.from(this._workers);

		return array[array.length * Math.random() << 0];
	}
}

module.exports = Server;
