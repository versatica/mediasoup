'use strict';

const debug = require('debug')('mediasoup:Worker');
const debugerror = require('debug')('mediasoup:ERROR:Worker');

debugerror.log = console.error.bind(console);

const path = require('path');
const spawn = require('child_process').spawn;
const process = require('process');
const EventEmitter = require('events').EventEmitter;

const WORKER_BIN_PATH = path.join(__dirname, '..', 'worker', 'bin', 'mediasoup-worker');

// Set an env variable to tell workers which the Control FD is
process.env.MEDIASOUP_CONTROL_FD = '3';

class Worker extends EventEmitter
{
	constructor(id, parameters)
	{
		super();

		debug('constructor() [id:%s]', id);

		let spawnArgs = [ id ].concat(parameters);
		let spawnOptions =
		{
			detached : false,
			stdio    : [ 'pipe', 'pipe', 'pipe', 'pipe' ]
		};

		// Create the worker child process
		this._child = spawn(WORKER_BIN_PATH, spawnArgs, spawnOptions);

		// TODO: TMP
		global.p = this._child.stdio[3];

		this._child.stdout.on('data', (buffer) =>
		{
			buffer.toString('utf8').split('\n').forEach((line) =>
			{
				if (line)
				{
					debug(line);
				}
			});
		});

		this._child.stderr.on('data', (buffer) =>
		{
			buffer.toString('utf8').split('\n').forEach((line) =>
			{
				if (line)
				{
					debugerror(line);
				}
			});
		});

		this._child.on('exit', (code, signal) =>
		{
			debug('worker process closed [id:%s, code:%s, signal:%s]', id, code, signal);

			this.emit('exit', code, signal);
		});

		this._child.on('error', (error) =>
		{
			debugerror('worker process error [id:%s]: %s', id, error);

			this.emit('error', error);
		});
	}

	close()
	{
		debug('close()');

		// Kill worker child
		this._child.kill('SIGTERM');
	}
}

module.exports = Worker;
