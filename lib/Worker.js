'use strict';

const path = require('path');
const spawn = require('child_process').spawn;
const process = require('process');
const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Worker');
const utils = require('./utils');
const Channel = require('./Channel');
const Room = require('./Room');

const WORKER_BIN_PATH = path.join(__dirname, '..', 'worker', 'bin', 'mediasoup-worker');
const CHANNEL_FD = 3;

// Set environtment variables for the worker
process.env.MEDIASOUP_CHANNEL_FD = String(CHANNEL_FD);

class Worker extends EventEmitter
{
	constructor(id, parameters)
	{
		logger.debug('constructor() [id:%s, parameters:"%s"]', id, parameters.join(' '));

		super();

		let spawnArgs = [ id ].concat(parameters);
		let spawnOptions =
		{
			detached : false,
			/*
			 * fd 0 (stdin)   : Just ignore it
			 * fd 1 (stdout)  : Pipe it for 3rd libraries in the worker
			 *                  that log their own stuff
			 * fd 2 (stderr)  : Same as stdout
			 * fd 3 (channel) : Channel fd
			 */
			stdio    : [ 'ignore', 'pipe', 'pipe', 'pipe' ]
		};

		// Create the mediasoup-worker child process
		this._child = spawn(WORKER_BIN_PATH, spawnArgs, spawnOptions);

		// Channel instance
		this._channel = new Channel(this._child.stdio[CHANNEL_FD]);

		// Set of Room instances
		this._rooms = new Set();

		// Be ready for 3rd party worker libraries logging to stdout/stderr

		this._child.stdout.on('data', (buffer) =>
		{
			buffer.toString('utf8').split('\f').forEach((line) =>
			{
				if (line)
					logger.debug(`mediasoup-worker's stdout [id:${id}]: ${line}`);
			});
		});

		this._child.stderr.on('data', (buffer) =>
		{
			buffer.toString('utf8').split('\f').forEach((line) =>
			{
				if (line)
					logger.error(`mediasoup-worker's stderr [id:${id}]: ${line}`);
			});
		});

		// Set child process events

		this._child.on('exit', (code, signal) =>
		{
			logger.error('child process exited [id:%s, code:%s, signal:%s]', id, code, signal);

			this._child = null;
			this.close(new Error(`child process exited [id:${id}, code:${code}, signal:${signal}]`));
		});

		this._child.on('error', (error) =>
		{
			logger.error('child process error [id:%s, error:%s]', id, error);

			this._child = null;
			this.close(new Error(`child process error [id:${id}, error:${error}]`));
		});
	}

	close(error)
	{
		if (!error)
			logger.debug('close()');
		else
			logger.error('close() [error:%s]', error);

		// Close every Room
		this._rooms.forEach((room) => room.close(undefined, true));

		// Kill mediasoup-worker process (let it some time so previous
		// requests are properly sent and replied)
		setTimeout(() =>
		{
			// Kill child
			if (this._child)
			{
				this._child.removeAllListeners('exit');
				this._child.removeAllListeners('error');
				this._child.kill('SIGTERM');
			}

			this._child = null;

			// Close the Channel instance
			this._channel.close();
		}, 100);

		this.emit('close', error);
	}

	dump()
	{
		logger.debug('dump()');

		return this._channel.request('dumpWorker')
			.then((data) =>
			{
				logger.debug('"dumpWorker" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"dumpWorker" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}

	updateSettings(options)
	{
		logger.debug('updateSettings() [options:%o]', options);

		return this._channel.request('updateSettings', options)
			.then(() =>
			{
				logger.debug('"updateSettings" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"updateSettings" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}

	createRoom(options)
	{
		logger.debug('createRoom() [options:%o]', options);

		options.roomId = utils.randomNumber();

		return this._channel.request('createRoom', options)
			.then(() =>
			{
				logger.debug('"createRoom" request succeeded');

				// Create a Room instance
				let room = new Room(options, this._channel);

				// Store the Room instance and remove it when closed
				this._rooms.add(room);
				room.on('close', () => this._rooms.delete(room));

				return room;
			})
			.catch((error) =>
			{
				logger.error('"createRoom" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}
}

module.exports = Worker;
