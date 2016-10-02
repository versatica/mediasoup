'use strict';

const EventEmitter = require('events').EventEmitter;
const netstring = require('netstring');
const logger = require('./logger')('Channel');
const workerLogger = require('./logger')('mediasoup-worker');
const utils = require('./utils');
const errors = require('./errors');

// netstring length for a 65536 bytes payload.
const NS_MAX_SIZE = 65543;
// Max time waiting for a response from the worker subprocess.
const REQUEST_TIMEOUT = 5000;

class Channel extends EventEmitter
{
	constructor(socket)
	{
		logger.debug('constructor()');

		super();
		this.setMaxListeners(Infinity);

		// Unix Socket instance.
		this._socket = socket;

		this._pendingSent = new Map();

		// Buffer for incomplete data received from the Channel's socket.
		this._recvBuffer = null;

		// Last binary notification received.
		this._lastBinaryNotification = null;

		// Read Channel responses/notifications from the worker.
		this._socket.on('data', (buffer) =>
		{
			if (!this._recvBuffer)
			{
				this._recvBuffer = buffer;
			}
			else
			{
				this._recvBuffer = Buffer.concat([ this._recvBuffer, buffer ], this._recvBuffer.length + buffer.length);

				if (this._recvBuffer.length > NS_MAX_SIZE)
				{
					logger.error('recvBuffer is full, discarding all the data in it');

					// Reset the recvBuffer and exit.
					this._recvBuffer = null;
					// Just in case.
					this._lastBinaryNotification = null;

					return;
				}
			}

			while (true) // eslint-disable-line no-constant-condition
			{
				let nsPayload;

				try
				{
					nsPayload = netstring.nsPayload(this._recvBuffer);
				}
				catch (error)
				{
					logger.error('invalid data received: %s', error.message);

					// Reset the recvBuffer and exit.
					this._recvBuffer = null;
					// Just in case.
					this._lastBinaryNotification = null;

					return;
				}

				// Incomplete netstring.
				if (nsPayload === -1)
				{
					return;
				}

				// We are not waiting for binary data for a previous binary event.
				if (!this._lastBinaryNotification)
				{
					try
					{
						// We can receive JSON messages (Channel messages) or log strings.
						switch (nsPayload[0])
						{
							// 123 = '{' (a Channel JSON messsage).
							case 123:
								this._processMessage(JSON.parse(nsPayload));
								break;

							// 68 = 'D' (a debug log).
							case 68:
								workerLogger.debug(nsPayload.toString(null, 1));
								break;

							// 87 = 'W' (a warning log).
							case 87:
								workerLogger.warn(nsPayload.toString(null, 1));
								break;

							// 69 = 'E' (an error log).
							case 69:
								workerLogger.error(nsPayload.toString(null, 1));
								break;

							default:
								workerLogger.error('unexpected data: %s', nsPayload.toString());
						}
					}
					catch (error)
					{
						logger.error('received invalid message: %s', error.message);
					}
				}
				// This is binary data for a previous binary event, so emit the full
				// binary event with its binary data.
				else
				{
					let msg = this._lastBinaryNotification;

					// Reset.
					this._lastBinaryNotification = null;
					// Emit.
					this.emit(msg.targetId, msg.event, msg.data, nsPayload);
				}

				// Remove the read payload from the recvBuffer.
				this._recvBuffer = this._recvBuffer.slice(netstring.nsLength(this._recvBuffer));

				if (!this._recvBuffer.length)
				{
					this._recvBuffer = null;

					return;
				}
			}
		});

		this._socket.on('end', () =>
		{
			logger.debug('channel ended by the other side');
		});

		this._socket.on('error', (error) =>
		{
			logger.error('channel error: %s', error);
		});
	}

	close()
	{
		logger.debug('close()');

		// Close every pending sent.
		this._pendingSent.forEach((sent) => sent.close());

		// Remove event listeners but leave a fake 'error' hander
		// to avoid propagation.
		this._socket.removeAllListeners('end');
		this._socket.removeAllListeners('error');
		this._socket.on('error', () => null);

		// Destroy the socket after a while to allow pending incoming
		// messages.
		setTimeout(() =>
		{
			try
			{
				this._socket.destroy();
			}
			catch (error)
			{}
		}, 250);
	}

	request(method, internal, data)
	{
		let id = utils.randomNumber();

		logger.debug('request() [method:%s, id:%s]', method, id);

		let request = { id, method, internal, data };
		let ns = netstring.nsWrite(JSON.stringify(request));

		if (Buffer.byteLength(ns) > NS_MAX_SIZE)
			return Promise.reject(new Error('request too big'));

		// This may raise if closed or remote side ended.
		try
		{
			this._socket.write(ns);
		}
		catch (error)
		{
			return Promise.reject(error);
		}

		return new Promise((pResolve, pReject) =>
		{
			let sent =
			{
				resolve: (data) =>
				{
					if (!this._pendingSent.delete(id))
						return;

					clearTimeout(sent.timer);
					pResolve(data);
				},

				reject: (error) =>
				{
					if (!this._pendingSent.delete(id))
						return;

					clearTimeout(sent.timer);
					pReject(error);
				},

				timer: setTimeout(() =>
				{
					if (!this._pendingSent.delete(id))
						return;

					pReject(new Error('request timeout'));
				}, REQUEST_TIMEOUT),

				close: () =>
				{
					clearTimeout(sent.timer);
					pReject(new errors.InvalidStateError('Channel closed'));
				}
			};

			// Add sent stuff to the Map.
			this._pendingSent.set(id, sent);
		});
	}

	_processMessage(msg)
	{
		logger.debug('received message: %o', msg);

		// If a Response, retrieve its associated Request.
		if (msg.id)
		{
			let sent = this._pendingSent.get(msg.id);

			if (!sent)
			{
				logger.error('received Response does not match any sent Request');

				return;
			}

			if (msg.accepted)
				sent.resolve(msg.data);
			else if (msg.rejected)
				sent.reject(new Error(msg.reason));
		}
		// If a Notification emit it to the corresponding entity.
		else if (msg.targetId && msg.event)
		{
			// If a binary notification, keep this message until the binary
			// data arrives.
			if (msg.binary)
			{
				this._lastBinaryNotification = msg;

				return;
			}

			this.emit(msg.targetId, msg.event, msg.data);
		}
		// Otherwise unexpected message.
		else
		{
			logger.error('received message is not a Response nor a Notification');
		}
	}
}

module.exports = Channel;
