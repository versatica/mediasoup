'use strict';

const debug = require('debug')('mediasoup:Worker');
const debugerror = require('debug')('mediasoup:ERROR:Worker');

debugerror.log = console.error.bind(console);

const path = require('path');
const spawn = require('child_process').spawn;
const process = require('process');
const EventEmitter = require('events').EventEmitter;

const Channel = require('./Channel');
const Room = require('./Room');

const WORKER_BIN_PATH = path.join(__dirname, '..', 'worker', 'bin', 'mediasoup-worker');
const CHANNEL_FD = 3;

// Set an env variable to tell childs which the Channel FD is
process.env.MEDIASOUP_CHANNEL_FD = String(CHANNEL_FD);

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

		// Create the mediasoup-worker child process
		this._child = spawn(WORKER_BIN_PATH, spawnArgs, spawnOptions);

		// Channel instance
		this._channel = new Channel(this._child.stdio[CHANNEL_FD]);

		// Set of Room instances
		this._rooms = new Set();

		// Set child process events

		this._child.stdout.on('data', (buffer) =>
		{
			buffer.toString('utf8').split('\n').forEach((line) => line && debug(line));
		});

		this._child.stderr.on('data', (buffer) =>
		{
			buffer.toString('utf8').split('\n').forEach((line) => line && debugerror(line));
		});

		this._child.on('exit', (code, signal) =>
		{
			debug('child process closed [id:%s, code:%s, signal:%s]', id, code, signal);

			// TODO: do something
		});

		this._child.on('error', (error) =>
		{
			debugerror('child process error [id:%s]: %s', id, error);

			// TODO: do something
		});
	}

	close()
	{
		debug('close()');

		// Kill child
		this._child.kill('SIGTERM');

		// Close every Room
		this._rooms.forEach(room => room.close());

		this.emit('close');
	}

	createRoom(options)
	{
		debug('createRoom()');

		let room = new Room(options, this._channel);

		// Store the Room instance and remove it when closed
		this._rooms.add(room);
		room.on('close', () => this._rooms.delete(room));

		return room;
	}
}

module.exports = Worker;
