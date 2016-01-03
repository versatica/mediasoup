'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Transport');
const utils = require('./utils');
const errors = require('./errors');

class Transport extends EventEmitter
{
	constructor(options, data, channel)
	{
		logger.debug('constructor() [options:%o, data:%o]', options, data);

		super();

		// Room identificator
		this._roomId = options.roomId;

		// Peer identificator
		this._peerId = options.peerId;

		// Transport identificator
		this._transportId = options.transportId;

		// Transport data provided by the worker
		this._data = data;

		// Channel instance
		this._channel = channel;

		// Closed flag
		this._closed = false;

		// TODO: subscribe to channel notifications from the worker
	}

	get closed()
	{
		return this._closed;
	}

	get iceComponent()
	{
		return this._data.iceComponent;
	}

	get iceLocalParameters()
	{
		return this._data.iceLocalParameters;
	}

	get iceLocalCandidates()
	{
		return this._data.iceLocalCandidates;
	}

	get dtlsLocalFingerprints()
	{
		return this._data.dtlsLocalFingerprints;
	}

	/**
	 * Close the Transport
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

		if (!dontSendChannel)
		{
			let data =
			{
				roomId      : this._roomId,
				peerId      : this._peerId,
				transportId : this._transportId
			};

			// Send Channel request
			this._channel.request('transport.close', data)
				.then(() =>
				{
					logger.debug('"transport.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"transport.close" request failed [status:%s, error:"%s"]', error.status, error.message);
				});
		}

		this.emit('close', error);
	}

	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(errors.Closed('transport closed'));

		let data =
		{
			roomId      : this._roomId,
			peerId      : this._peerId,
			transportId : this._transportId
		};

		return this._channel.request('transport.dump', data)
			.then((data) =>
			{
				logger.debug('"transport.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"transport.dump" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}

	/**
	 * Start the DTLS stack
	 * @param  {Object} remoteDtlsParameters  Remote DTLS parameters
	 * @return {Promise}
	 */
	start(remoteDtlsParameters)
	{
		logger.debug('start() [remoteDtlsParameters:%o]', remoteDtlsParameters);

		if (this._closed)
			return Promise.reject(errors.Closed('transport closed'));

		let data =
		{
			roomId               : this._roomId,
			peerId               : this._peerId,
			transportId          : this._transportId,
			role                 : remoteDtlsParameters.role,
			fingerprint          : remoteDtlsParameters.fingerprint
		};

		// Send Channel request
		this._channel.request('transport.start', data)
			.then(() =>
			{
				logger.debug('"transport.start" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"transport.start" request failed [status:%s, error:"%s"]', error.status, error.message);
			});
	}

	/**
	 * Create an asociated Transport for RTCP
	 * @return {Promise}
	 */
	createAssociatedTransport()
	{
		logger.debug('createAssociatedTransport()');

		if (this._closed)
			return Promise.reject(errors.Closed('transport closed'));

		let options =
		{
			roomId         : this._roomId,
			peerId         : this._peerId,
			transportId    : utils.randomNumber(),
			rtpTransportId : this._transportId
		};

		return this._channel.request('peer.createAssociatedTransport', options)
			.then((data) =>
			{
				logger.debug('"peer.createAssociatedTransport" request succeeded');

				// Create a Transport instance
				let transport = new Transport(options, data, this._channel);

				// Emit 'associatedtransport' so the owner Peer can handle this new
				// transport
				this.emit('associatedtransport', transport);

				return transport;
			})
			.catch((error) =>
			{
				logger.error('"peer.createAssociatedTransport" request failed [status:%s, error:"%s"]', error.status, error.message);

				throw error;
			});
	}
}

module.exports = Transport;
