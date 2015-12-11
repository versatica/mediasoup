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
const merge = require('merge');

const WORKER_BIN_PATH = path.join(__dirname, '..', '..', 'mediasoup-worker', 'bin', 'mediasoup-worker');
const DEFAULT_NUM_WORKERS = Object.keys(os.cpus()).length;
const DEFAULT_OPTIONS =
{
	logLevel            : 'info',
	useSyslog           : false,
	syslogFacility      : 'user',
	rtcListenIPv4       : true,
	rtcListenIPv6       : true,
	rtcMinPort          : 10000,
	rtcMaxPort          : 59999,
	dtlsCertificateFile : null,
	dtlsPrivateKeyFile  : null
};
const VALID_OPTIONS =
{
	logLevel       : [ 'debug', 'info', 'notice', 'warn', 'error' ],
	syslogFacility : [ 'user', 'local0', 'local1', 'local2', 'local3', 'local4', 'local5', 'local6', 'local7' ]
};

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

		options = merge(true, DEFAULT_OPTIONS, options);

		if (check.integer(options.numWorkers) && check.positive(options.numWorkers))
		{
			numWorkers = options.numWorkers;
		}

		if (!check.includes(VALID_OPTIONS.logLevel, options.logLevel))
		{
			throw new Error(`invalid option value [logLevel:${options.logLevel}]`);
		}

		if (!check.boolean(options.useSyslog))
		{
			throw new Error(`invalid option value [useSyslog:${options.useSyslog}]`);
		}

		if (!check.includes(VALID_OPTIONS.syslogFacility, options.syslogFacility))
		{
			throw new Error(`invalid option value [syslogFacility:${options.syslogFacility}]`);
		}

		if (!check.integer(options.rtcMinPort) || !check.positive(options.rtcMinPort))
		{
			throw new Error(`invalid option value [rtcMinPort:${options.rtcMinPort}]`);
		}

		if (!check.integer(options.rtcMaxPort) || !check.positive(options.rtcMaxPort))
		{
			throw new Error(`invalid option value [rtcMaxPort:${options.rtcMaxPort}]`);
		}

		// Map of mediasoup-worker child processes
		this._workers = new Map();

		// Create mediasoup-worker child processes
		for (let i = 1; i <= numWorkers; i++)
		{
			let workerId = serverId + '#' + i;
			let worker;
			let workerOptions = merge(true, options);
			let parameters = [];

			for (let key of Object.keys(workerOptions))
			{
				if (DEFAULT_OPTIONS.hasOwnProperty(key))
				{
					let value = String(workerOptions[key]);

					parameters.push(`--${key}=${value}`);
				}
			}

			debug('constructor() creating worker [workerId:%s, parameters:"%s"]', workerId, parameters.join(' '));

			worker = spawn(WORKER_BIN_PATH, [ workerId ].concat(parameters));

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

			worker.on('exit', (code) =>
			{
				debug('worker process closed [workerId:%s, code:%s]', worker.workerId, code);
			});

			worker.on('error', (error) =>
			{
				debugerror('worker process error [workerId:%s]: %s', worker.workerId, error);
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
