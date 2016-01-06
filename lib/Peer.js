'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Peer');
const utils = require('./utils');
const errors = require('./errors');
const Transport = require('./Transport');

class Peer extends EventEmitter
{
	constructor(internal, channel)
	{
		logger.debug('constructor() [internal:%o]', internal);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data
		this._internal = internal;

		// Channel instance
		this._channel = channel;

		// Set of Transport instances
		this._transports = new Set();

		// Closed flag
		this._closed = false;

		// TODO: subscribe to channel notifications from the worker
	}

	get closed()
	{
		return this._closed;
	}

	/**
	 * Close the Peer
	 */
	close(error, dontSendChannel)
	{
		if (!error)
			logger.debug('close()');
		else
			logger.error('close() [error:%s]', error);

		if (this._closed)
			return;

		this._closed = true;

		// Close every Transport
		this._transports.forEach((transport) => transport.close(undefined, true));

		if (!dontSendChannel)
		{
			let internal = this._internal;

			// Send Channel request
			this._channel.request('peer.close', internal)
				.then(() =>
				{
					logger.debug('"peer.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"peer.close" request failed [status:%s, error:"%s"]', error.status, error.message);
				});
		}

		this.emit('close', error);
	}

	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(errors.Closed('peer closed'));

		let internal = this._internal;

		return this._channel.request('peer.dump', internal)
			.then((data) =>
			{
				logger.debug('"peer.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"peer.dump" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}

	/**
	 * Create a Transport
	 * @param  {Object} options  Transport options
	 * @param  {Boolean} options.udp  Listen UDP (default true)
	 * @param  {Boolean} options.tcp  Listen TCP (default true)
	 * @return {Promise}
	 */
	createTransport(options)
	{
		logger.debug('createTransport() [options:%o]', options);

		if (this._closed)
			return Promise.reject(errors.Closed('peer closed'));

		let internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			transportId : utils.randomNumber()
		};

		return this._channel.request('peer.createTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"peer.createTransport" request succeeded');

				// Create a Transport instance
				let transport = new Transport(internal, data, this._channel);

				// Store the Transport instance and remove it when closed
				this._transports.add(transport);
				transport.on('close', () => this._transports.delete(transport));

				// If the user calls transport.createAssociatedTransport() handle it
				transport.on('associatedtransport', (associatedTransport) =>
				{
					// Store the Transport instance and remove it when closed
					this._transports.add(associatedTransport);
					associatedTransport.on('close', () => this._transports.delete(associatedTransport));
				});

				return transport;
			})
			.catch((error) =>
			{
				logger.error('"peer.createTransport" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}
}

module.exports = Peer;
