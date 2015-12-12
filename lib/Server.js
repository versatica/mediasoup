'use strict';

const debug = require('debug')('mediasoup:Server');
const debugerror = require('debug')('mediasoup:ERROR:Server');

debugerror.log = console.error.bind(console);

const process = require('process');
const os = require('os');
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

		if (check.integer(options.numWorkers) && check.positive(options.numWorkers))
		{
			numWorkers = options.numWorkers;
		}

		// Map of Worker instances
		this._workers = new Map();

		// Add the server to the set
		servers.add(this);

		for (let key of Object.keys(options))
		{
			if (check.includes(VALID_PARAMETERS, key))
			{
				parameters.push(`--${key}=${String(options[key])}`);
			}
		}

		debug('constructor() [worker parameters:"%s"]', parameters.join(' '));

		// Create mediasoup-worker child processes
		for (let i = 1; i <= numWorkers; i++)
		{
			let worker;
			let workerId = serverId + '#' + i;

			// Create a Worker instance
			worker = new Worker(workerId, parameters);

			worker.on('exit', () =>
			{
				this._workers.delete(workerId);
			});

			worker.on('error', () =>
			{
				this._workers.delete(workerId);
			});

			// Add the worker to the map
			this._workers.set(workerId, worker);
		}
	}

	close()
	{
		debug('close()');

		// Remove this server from the set
		servers.delete(this);

		// Close every worker
		for (let worker of this._workers.values())
		{
			worker.close();
		}

		// Clear the workers map
		this._workers.clear();
	}
}

module.exports = Server;
