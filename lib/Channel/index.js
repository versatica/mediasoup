'use strict';

const EventEmitter = require('events').EventEmitter;
const netstring = require('netstring');
const logger = require('../logger')('Channel');
const Request = require('./Request');

// Max time waiting for a response from the worker subprocess
const MESSAGE_MAX_SIZE = 65536;
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
				// TODO: remove
				// logger.debug('recvBuffer was empty');

				this._recvBuffer = buffer;
			}
			else
			{
				// TODO: remove
				// logger.debug('recvBuffer was not empty [length:%s]', this._recvBuffer.length);

				this._recvBuffer = Buffer.concat([ this._recvBuffer, buffer ], this._recvBuffer.length + buffer.length);

				if (this._recvBuffer.length > MESSAGE_MAX_SIZE)
				{
					logger.error('recvBuffer full, discarding all the data in it');

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
					logger.error('invalid JSON received: %s', error.message);

					return;
				}

				// Process JSON
				this._processMessage(json);

				// Remove the read payload from the recvBuffer
				this._recvBuffer = this._recvBuffer.slice(netstring.nsLength(this._recvBuffer));

				if (!this._recvBuffer.length)
				{
					// TODO: remove
					// logger.debug('no more data in the buffer');

					this._recvBuffer = null;
					return;
				}
			}
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

	sendRequest(method, data)
	{
		logger.debug('sendRequest() [method:%s]', method);

		let request = new Request(method, data);
		let ns = netstring.nsWrite(request.serialize());
		let promise;

		// This may raise if closed or remote side ended
		try
		{
			this._socket.write(ns);
		}
		catch (error)
		{
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

					error.name = 'TIMEOUT';
					pReject(error);
				}, REQUEST_TIMEOUT),

				close: () =>
				{
					let error = new Error('channel closed');

					error.name = 'CHANNEL_CLOSED';
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
		logger.debug('_processMessage()');

		// TODO: remove
		logger.debug('received json: %o', json);

		// If a Response retrieve its associated Request
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
				sent.reject(json.data);
			}
		}
	}
}

module.exports = Channel;
