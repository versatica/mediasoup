'use strict';

const debug = require('debug')('mediasoup:Server');
const debugerror = require('debug')('mediasoup:ERROR:Server');

debugerror.log = console.error.bind(console);

const process = require('process');
const os = require('os');
const path = require('path');
const check = require('check-types');
const randomString = require('random-string');

const Worker = require('./Worker');

const DEFAULT_NUM_WORKERS = Object.keys(os.cpus()).length;
const VALID_PARAMETERS =
[
	'logLevel', 'rtcListenIPv4', 'rtcListenIPv6',	'rtcMinPort', 'rtcMaxPort',
	'dtlsCertificateFile', 'dtlsPrivateKeyFile'
];

// Set of Server instances
var servers = new Set();

// On process exit close all the Servers
process.on('exit', () =>
{
	for (let server of servers)
	{
		server.close();
	}
});

class Server
{
	constructor(options)
	{
		debug('constructor() [options:%o]', options);

		let serverId = randomString({ numeric: false, length: 6 }).toLowerCase();
		let numWorkers = DEFAULT_NUM_WORKERS;
		let parameters = [];

		options = options || {};

		// Map of Worker instances
		this._workers = new Map();

		// Map of Room instances
		this._rooms = new Map();

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

		// Add this Server to the Set
		servers.add(this);

		for (let key of Object.keys(options))
		{
			if (check.includes(VALID_PARAMETERS, key))
			{
				parameters.push(`--${key}=${String(options[key])}`);
			}
		}

		debug('constructor() [worker parameters:"%s"]', parameters.join(' '));

		// Create Worker instances
		for (let i = 1; i <= numWorkers; i++)
		{
			let worker;
			let workerId = serverId + '#' + i;

			worker = new Worker(workerId, parameters);

			worker.on('exit', () =>
			{
				this._workers.delete(workerId);
			});

			worker.on('error', () =>
			{
				this._workers.delete(workerId);
			});

			// Add the Worker to the Map
			this._workers.set(workerId, worker);
		}
	}

	close()
	{
		debug('close()');

		// Remove this Server from the Set
		servers.delete(this);

		// Close every Worker
		for (let worker of this._workers.values())
		{
			worker.close();
		}
		this._workers.clear();
	}

	createRoom(options)
	{
		debug('createRoom()');

		let worker = this._getRandomWorker();

		return worker.createRoom(options);
	}

	_getRandomWorker()
	{
		let workerIds = Array.from(this._workers.keys());

		return this._workers.get(workerIds[workerIds.length * Math.random() << 0]);
	}
}

module.exports = Server;
