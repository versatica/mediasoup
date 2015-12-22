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
const LOGGER_FD = 4;

// Set environtemt variables for the worker
process.env.MEDIASOUP_CHANNEL_FD = String(CHANNEL_FD);
process.env.MEDIASOUP_LOGGER_FD = String(LOGGER_FD);

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
			 * fd 4 (logger)  : Logger fd
			 */
			stdio    : [ 'ignore', 'pipe', 'pipe', 'pipe', 'pipe' ]
		};

		// Create the mediasoup-worker child process
		this._child = spawn(WORKER_BIN_PATH, spawnArgs, spawnOptions);

		// Channel instance
		this._channel = new Channel(this._child.stdio[CHANNEL_FD]);

		// Set of Room instances
		this._rooms = new Set();

		// Set worker logger
		this._child.stdio[LOGGER_FD].on('data', (buffer) =>
		{
			buffer.toString('utf8').split('\f').forEach((line) =>
			{
				switch (line[0])
				{
					case undefined:
						break;
					// MS_DEBUG & MS_TRACE
					case 'D':
						logger.debug(line.slice(1));
						break;
					// MS_WARN
					case 'W':
						logger.warn(line.slice(1));
						break;
					// MS_ERROR
					case 'E':
						logger.error(line.slice(1));
						break;
					// Should not happen
					default:
						logger.warn(`unexpected log: ${line}`);
				}
			});
		});

		// Be also ready for 3rd party worker libraries logging to stdout/stderr

		this._child.stdout.on('data', (buffer) =>
		{
			logger.debug(`worker stdout: ${buffer.toString('utf8')}`);
		});

		this._child.stderr.on('data', (buffer) =>
		{
			logger.error(`worker stderr: ${buffer.toString('utf8')}`);
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
		logger.debug('close() [error:%s]', error);

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

		// Close every Room
		this._rooms.forEach((room) => room.close());

		this.emit('close', error);
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
				var room = new Room(options, this._channel);

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
