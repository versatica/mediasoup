'use strict';

const EventEmitter = require('events').EventEmitter;
const netstring = require('netstring');
const logger = require('../logger')('Channel');
const Request = require('./Request');

// Max time waiting for a response from the worker subprocess
const REQUEST_TIMEOUT = 4000;  // TODO: set it properly

class Channel extends EventEmitter
{
	constructor(socket)
	{
		super();

		logger.debug('constructor()');

		// Unix Socket instance
		this._socket = socket;

		this._pendingSent = new Map();

		// TODO: read Channel responses/notifications from the worker

		this._socket.on('data', (buffer) =>
		{
			logger.debug('data received from the socket: %s', buffer.toString('utf8'));
		});
	}

	close()
	{
		logger.debug('close()');

		// Close every pending sent
		this._pendingSent.forEach(sent => sent.close());

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
}

module.exports = Channel;
