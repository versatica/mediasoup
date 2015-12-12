'use strict';

const path = require('path');
const spawn = require('child_process').spawn;
const process = require('process');
const os = require('os');

const debug = require('debug')('mediasoup:Server');
const debugerror = require('debug')('mediasoup:ERROR:Server');

debugerror.log = console.error.bind(console);

const check = require('check-types');
const randomString = require('random-string');

const WORKER_BIN_PATH = path.join(__dirname, '..', 'worker', 'bin', 'mediasoup-worker');
const DEFAULT_NUM_WORKERS = Object.keys(os.cpus()).length;
const VALID_PARAMETERS =
[
	'logLevel', 'rtcListenIPv4', 'rtcListenIPv6',	'rtcMinPort', 'rtcMaxPort',
	'dtlsCertificateFile', 'dtlsPrivateKeyFile'
];

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

		// Map of mediasoup-worker child processes
		this._workers = new Map();

		for (let key of Object.keys(options))
		{
			if (check.includes(VALID_PARAMETERS, key))
			{
				parameters.push(`--${key}=${String(options[key])}`);
			}
		}

		debug('constructor() | [worker parameters:"%s"]', parameters.join(' '));

		// Add the server to the set
		servers.add(this);

		// Create mediasoup-worker child processes
		for (let i = 1; i <= numWorkers; i++)
		{
			let worker;
			let workerId = serverId + '#' + i;
			let spawnArgs = [ workerId ].concat(parameters);
			let spawnOptions =
			{
				detached : false,
				stdio    : [ 'pipe', 'pipe', 'pipe', 'ipc' ]
			};

			debug('constructor() | creating worker [workerId:%s]', workerId);

			// Create the worker child process
			worker = spawn(WORKER_BIN_PATH, spawnArgs, spawnOptions);

			// Store worker id within the worker process
			worker.workerId = workerId;

			worker.stdout.on('data', (buffer) =>
			{
				buffer.toString('utf8').split('\n').forEach((line) =>
				{
					if (line)
					{
						debug(line);
					}
				});
			});

			worker.stderr.on('data', (buffer) =>
			{
				buffer.toString('utf8').split('\n').forEach((line) =>
				{
					if (line)
					{
						debugerror(line);
					}
				});
			});

			worker.on('exit', (code, signal) =>
			{
				debug('worker process closed [workerId:%s, code:%s, signal:%s]', worker.workerId, code, signal);

				this._workers.delete(worker.workerId);
			});

			worker.on('error', (error) =>
			{
				debugerror('worker process error [workerId:%s]: %s', worker.workerId, error);
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

		// Kill every worker process
		for (let worker of this._workers.values())
		{
			worker.kill('SIGTERM');
		}

		// Clear the workers map
		this._workers.clear();
	}
}

module.exports = Server;
