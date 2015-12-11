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

const DEFAULT_NUM_WORKERS = Object.keys(os.cpus()).length;
const MEDIASOUP_WORKER_BIN_PATH = path.join(__dirname, '..', '..', 'mediasoup-worker', 'bin', 'mediasoup-worker');

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
		let numWorkers;

		options = options || {};

		if (check.integer(options.numWorkers) && check.positive(options.numWorkers))
		{
			numWorkers = options.numWorkers;
		}
		else
		{
			numWorkers = DEFAULT_NUM_WORKERS;
		}

		debug('constructor() [serverId:%s, numWorkers:%d]', serverId, numWorkers);

		// mediasoup-worker child processes within this room
		this._workers = new Map();

		// Create mediasoup worker child processes
		for (let i = 1; i <= numWorkers; i++)
		{
			let workerId = serverId + '#' + i;
			let worker = spawn(MEDIASOUP_WORKER_BIN_PATH, [ workerId ]);

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

			worker.on('close', (code) =>
			{
				debug('worker process closed [workerId:%s, code:%s]', worker.workerId, code);
			});

			worker.on('error', (error) =>
			{
				debugerror('worker process failed to start [workerId:%s]: %s', worker.workerId, error);
			});

			// Add the worker to the map
			this._workers.set(workerId, worker);
		}

		// Add the server to the set
		servers.add(this);
	}

	close()
	{
		debug('close()');

		for (let worker of this._workers.values())
		{
			// TODO: set proper signal
			worker.kill();
		}
	}
}

module.exports = Server;
