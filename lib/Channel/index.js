'use strict';

const EventEmitter = require('events').EventEmitter;
const netstring = require('netstring');

const logger = require('../logger')('Channel');
const Request = require('./Request');

// netstring length for a 65536 bytes payload
const NS_MAX_SIZE = 65543;
// Max time waiting for a response from the worker subprocess
const REQUEST_TIMEOUT = 5000;

class Channel extends EventEmitter
{
	constructor(socket)
	{
		super();

		logger.debug('constructor()');

		// Unix Socket instance
		this._socket = socket;

		this._pendingSent = new Map();

		// Buffer for incomplete data received from the Channel's socket
		this._recvBuffer = null;

		// Read Channel responses/notifications from the worker
		this._socket.on('data', (buffer) =>
		{
			// No recvBuffer
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

					// Reset the recvBuffer and exit
					this._recvBuffer = null;
					return;
				}
			}

			while (true)
			{
				let nsPayload;
				let json;

				try
				{
					nsPayload = netstring.nsPayload(this._recvBuffer);
				}
				catch (error)
				{
					logger.error('invalid data received: %s', error.message);

					// Reset the recvBuffer and exit
					this._recvBuffer = null;
					return;
				}

				// Incomplete netstring
				if (nsPayload === -1)
				{
					logger.debug('not enought data, waiting for more data');

					return;
				}

				// Check whether it is valid JSON
				try
				{
					// NOTE: cool, JSON.parse() allows a Buffer
					json = JSON.parse(nsPayload);
				}
				catch (error)
				{
					logger.error('received invalid JSON: %s', error.message);

					return;
				}

				// Process JSON
				this._processMessage(json);

				// Remove the read payload from the recvBuffer
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

		// Close every pending sent
		this._pendingSent.forEach((sent) => sent.close());

		// Close the UnixStream socket
		this._socket.destroy();
	}

	request(method, data)
	{
		logger.debug('request() [method:%s]', method);

		let request = new Request(method, data);
		let ns = netstring.nsWrite(request.serialize());
		let promise;

		if (Buffer.byteLength(ns) > NS_MAX_SIZE)
		{
			let error = new Error('message too big');

			// Add .status=3XX to the Error instance
			error.status = 313;
			return Promise.reject(error);
		}

		// This may raise if closed or remote side ended
		try
		{
			this._socket.write(ns);
		}
		catch (error)
		{
			// Add .status=3XX to the Error instance
			error.status = 300;
			return Promise.reject(error);
		}

		promise = new Promise((pResolve, pReject) =>
		{
			let sent =
			{
				resolve: (data) =>
				{
					if (!this._pendingSent.delete(request.id))
					{
						return;
					}

					clearTimeout(sent.timer);
					pResolve(data);
				},

				reject: (error) =>
				{
					if (!this._pendingSent.delete(request.id))
					{
						return;
					}

					clearTimeout(sent.timer);
					pReject(error);
				},

				timer: setTimeout(() =>
				{
					if (!this._pendingSent.delete(request.id))
					{
						return;
					}

					let error = new Error('timeout');

					// Add .status=3XX to the Error instance
					error.status = 308;
					pReject(error);
				}, REQUEST_TIMEOUT),

				close: () =>
				{
					let error = new Error('channel closed');

					// Add .status=3XX to the Error instance
					error.status = 310;
					clearTimeout(sent.timer);
					pReject(error);
				}
			};

			// Add sent stuff to the Map
			this._pendingSent.set(request.id, sent);
		});

		return promise;
	}

	_processMessage(json)
	{
		logger.debug('received message: %o', json);

		// If a Response, retrieve its associated Request
		if (json.id)
		{
			let sent = this._pendingSent.get(json.id);

			if (!sent)
			{
				logger.error('received Response does not match any sent Request');

				return;
			}

			if (json.status === 200)
			{
				sent.resolve(json.data);
			}
			else
			{
				let error = new Error(json.reason);

				// Add .status to the Error instance
				error.status = json.status;
				sent.reject(error);
			}
		}
		// If a Notification emit it to the corresponding entity
		else
		{
			// TODO
		}
	}
}

module.exports = Channel;
