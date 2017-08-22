'use strict';

const path = require('path');
const spawn = require('child_process').spawn;
const process = require('process');
const EventEmitter = require('events').EventEmitter;
const logger = require('./logger')('Worker');
const utils = require('./utils');
const Channel = require('./Channel');
const Room = require('./Room');

const CHANNEL_FD = 3;

let workerPath;

// If env MEDIASOUP_WORKER_PATH is given, use it.
if (process.env.MEDIASOUP_WORKER_PATH)
{
	workerPath = process.env.MEDIASOUP_WORKER_PATH;
}
// Otherwise check if env MEDIASOUP_BUILDTYPE == 'Debug' is set.
else if (process.env.MEDIASOUP_BUILDTYPE === 'Debug')
{
	workerPath = path.join(__dirname, '..', 'worker', 'out', 'Debug', 'mediasoup-worker');
}
// Otherwise use 'Release'.
else
{
	workerPath = path.join(__dirname, '..', 'worker', 'out', 'Release', 'mediasoup-worker');
}

// Set environtment variable for the worker.
process.env.MEDIASOUP_CHANNEL_FD = String(CHANNEL_FD);

class Worker extends EventEmitter
{
	constructor(id, parameters)
	{
		logger.debug('constructor() [id:%s, parameters:"%s"]', id, parameters.join(' '));

		super();
		this.setMaxListeners(Infinity);

		const spawnArgs = [ id ].concat(parameters);
		const spawnOptions =
		{
			detached : false,

			/*
			 * fd 0 (stdin)   : Just ignore it.
			 * fd 1 (stdout)  : Pipe it for 3rd libraries in the worker.
			 *                  that log their own stuff.
			 * fd 2 (stderr)  : Same as stdout.
			 * fd 3 (channel) : Channel fd.
			 */
			stdio : [ 'ignore', 'pipe', 'pipe', 'pipe' ]
		};

		// Create the mediasoup-worker child process.
		this._child = spawn(workerPath, spawnArgs, spawnOptions);

		// Channel instance.
		this._channel = new Channel(this._child.stdio[CHANNEL_FD]);

		// Set of Room instances.
		this._rooms = new Set();

		// Be ready for 3rd party worker libraries logging to stdout.
		this._child.stdout.on('data', (buffer) =>
		{
			for (const line of buffer.toString('utf8').split('\n'))
			{
				if (line)
					logger.debug(`mediasoup-worker's stdout: ${line}`);
			}
		});

		// In case of a worker bug, mediasoup will call abort() and log to stderr.
		this._child.stderr.on('data', (buffer) =>
		{
			for (const line of buffer.toString('utf8').split('\n'))
			{
				if (line)
				{
					// eslint-disable-next-line no-console
					console.error(`mediasoup-worker's stderr: ${line}`);
				}
			}
		});

		// Set child process events.

		this._child.on('exit', (code, signal) =>
		{
			logger.error('child process exited [id:%s, code:%s, signal:%s]', id, code, signal);

			this._child = null;
			this.close(new Error(
				`child process exited [id:${id}, code:${code}, signal:${signal}]`));
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

		// Close every Room.
		for (const room of this._rooms)
		{
			room.close(undefined, true);
		}

		// Kill mediasoup-worker process.
		if (this._child)
		{
			// Remove event listeners but leave a fake 'error' hander
			// to avoid propagation.
			this._child.removeAllListeners('exit');
			this._child.removeAllListeners('error');
			this._child.on('error', () => null);
			this._child.kill('SIGTERM');
		}

		this._child = null;

		// Close the Channel instance.
		this._channel.close();

		this.emit('close', error);
	}

	dump()
	{
		logger.debug('dump()');

		return this._channel.request('worker.dump')
			.then((data) =>
			{
				logger.debug('"worker.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"worker.dump" request failed: %s', error);

				throw error;
			});
	}

	updateSettings(options)
	{
		logger.debug('updateSettings() [options:%o]', options);

		return this._channel.request('worker.updateSettings', null, options)
			.then(() =>
			{
				logger.debug('"worker.updateSettings" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"worker.updateSettings" request failed: %s', error);

				throw error;
			});
	}

	createRoom(options)
	{
		logger.debug('createRoom() [options:%o]', options);

		const internal =
		{
			roomId : utils.randomNumber()
		};

		return this._channel.request('worker.createRoom', internal, options)
			.then((data) =>
			{
				logger.debug('"worker.createRoom" request succeeded');

				// Create a Room instance.
				const room = new Room(internal, data, this._channel);

				// Store the Room instance and remove it when closed.
				this._rooms.add(room);
				room.on('close', () => this._rooms.delete(room));

				return room;
			})
			.catch((error) =>
			{
				logger.error('"worker.createRoom" request failed: %s', error);

				throw error;
			});
	}
}

module.exports = Worker;
