'use strict';

const EventEmitter = require('events').EventEmitter;
const Logger = require('./Logger');
const utils = require('./utils');
const errors = require('./errors');
const Transport = require('./Transport');
const Producer = require('./Producer');
const Consumer = require('./Consumer');

const KINDS = [ 'audio', 'video', 'depth' ];

const logger = new Logger('Peer');

class Peer extends EventEmitter
{
	constructor(internal, channel, sandbox)
	{
		logger.debug('constructor() [internal:%o]', internal);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data.
		// - .roomId
		// - .peerId
		// - .peerName
		this._internal = internal;

		// Store internal sandbox stuff.
		// - .getProducerById()
		this._sandbox = sandbox;

		// Peer data.
		// - .rtpCapabilities
		this._data = {};

		// Channel instance.
		this._channel = channel;

		// Set of Transport instances.
		this._transports = new Set();

		// Set of Producer instances.
		this._producers = new Set();

		// Set of Consumer instances.
		this._consumers = new Set();

		// Closed flag.
		this._closed = false;

		// Subscribe to notifications.
		this._channel.on(this._internal.peerId, (event, data) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, true);
					break;
				}

				case 'newconsumer':
				{
					const associatedProducer = this._sandbox.getProducerById(
						data.associatedProducerId);

					const internal2 =
					{
						roomId      : this._internal.roomId,
						peerId      : this._internal.peerId,
						consumerId  : data.consumerId,
						transportId : null
					};

					const data2 =
					{
						kind               : data.kind,
						peer               : this,
						rtpParameters      : data.rtpParameters,
						associatedProducer : associatedProducer,
						active             : data.active
					};

					// Create a Consumer instance.
					const consumer = new Consumer(internal2, data2, this._channel);

					// Store the Consumer instance and remove it when closed.
					this._consumers.add(consumer);
					consumer.on('close', () => this._consumers.delete(consumer));

					this.emit('newconsumer', consumer);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}

	get id()
	{
		return this._internal.peerId;
	}

	get closed()
	{
		return this._closed;
	}

	get name()
	{
		return this._internal.peerName;
	}

	get rtpCapabilities()
	{
		return this._data.rtpCapabilities;
	}

	/**
	 * Get an array with all the Transports.
	 *
	 * @return {Array<Transports>}
	 */
	get transports()
	{
		return Array.from(this._transports);
	}

	/**
	 * Get an array with all the Producers.
	 *
	 * @return {Array<Producers>}
	 */
	get producers()
	{
		return Array.from(this._producers);
	}

	/**
	 * Get an array with all the Consumer.
	 *
	 * @return {Array<Consumer>}
	 */
	get consumers()
	{
		return Array.from(this._consumers);
	}

	/**
	 * Close the Peer.
	 */
	close(error, dontSendChannel)
	{
		if (this._closed)
			return;

		this._closed = true;

		if (!error)
			logger.debug('close()');
		else
			logger.error('close() [error:%s]', error);

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.peerId);

		// Close every Producer.
		for (const producer of this._producers)
		{
			producer.close(undefined, true);
		}

		// Close every Consumer.
		for (const consumer of this._consumers)
		{
			consumer.close();
		}

		// Close every Transport.
		for (const transport of this._transports)
		{
			transport.close(undefined, true);
		}

		if (!dontSendChannel)
		{
			// Send Channel request.
			this._channel.request('peer.close', this._internal)
				.then(() =>
				{
					logger.debug('"peer.close" request succeeded');
				})
				.catch((error2) =>
				{
					logger.error('"peer.close" request failed: %s', error2);
				});
		}

		this.emit('close', error);
	}

	/**
	 * Dump the Peer.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		return this._channel.request('peer.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"peer.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"peer.dump" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Set peer RTP capabilities.
	 *
	 * @param {RTCRtpCapabilities} [rtpCapabilities]
	 */
	setRtpCapabilities(rtpCapabilities)
	{
		logger.debug('setRtpCapabilities() [rtpCapabilities:%o]', rtpCapabilities);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		this._data.rtpCapabilities = rtpCapabilities;
	}

	/**
	 * Create a Producer instance.
	 *
	 * @param {Transport} transport - Transport instance.
	 *
	 * @return {Producer}
	 */
	Producer(kind, transport)
	{
		logger.debug('Producer() [kind:%s]', kind);

		if (this._closed)
			throw new errors.InvalidStateError('Peer closed');
		else if (!this.rtpCapabilities)
			throw new errors.InvalidStateError('RTP capabilities not set');
		else if (KINDS.indexOf(kind) === -1)
			throw new TypeError(`unsupported kind: ${kind}`);
		else if (!transport)
			throw new TypeError('transport not given');
		else if (!(transport instanceof Transport))
			throw new TypeError('transport must be a instance of Transport');
		else if (transport.closed)
			throw new errors.InvalidStateError('transport is closed');
		else if (!this._transports.has(transport))
			throw new Error('transport not found');

		const internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			producerId  : utils.randomNumber(),
			transportId : transport._internal.transportId
		};

		const data =
		{
			kind      : kind,
			peer      : this,
			transport : transport
		};

		// Create a Producer instance.
		const producer = new Producer(internal, data, this._channel);

		// Store the Producer instance and remove it when closed.
		this._producers.add(producer);
		producer.on('close', () => this._producers.delete(producer));

		const options =
		{
			kind : kind
		};

		this._channel.request('peer.createProducer', internal, options)
			.then(() =>
			{
				logger.debug('"peer.createProducer" request succeeded');

				this.emit('newproducer', producer);
			})
			.catch((error) =>
			{
				logger.error('"peer.createProducer" request failed: %s', error);

				producer.close(error, true);
			});

		return producer;
	}

	receiveRequest(request)
	{
		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Room closed'));
		else if (typeof request !== 'object')
			return Promise.reject(new TypeError('wrong request Object'));
		else if (request.notification)
			return Promise.reject(new TypeError('not a request'));
		else if (typeof request.method !== 'string')
			return Promise.reject(new TypeError('wrong/missing request method'));

		const { method } = request;

		logger.debug('receiveRequest() [method:%s]', method);

		return Promise.resolve()
			.then(() =>
			{
				switch (method)
				{
					// TODO: Handle appData.
					case 'createTransport':
					{
						return this._createTransport(
							{
								id                   : request.id,
								options              : request.options,
								remoteDtlsParameters : request.dtlsParameters
							})
							.then((transport) =>
							{
								const response =
								{
									iceParameters  : transport.iceLocalParameters,
									iceCandidates  : transport.iceLocalCandidates,
									dtlsParameters : transport.dtlsLocalParameters
								};

								return response;
							});
					}

					// TODO: Handle appData.
					case 'createProducer':
					{
						// TODO

						return;
					}

					default:
						throw new Error(`unknown request method "${method}"`);
				}
			})
			.catch((error) =>
			{
				logger.error(
					'receiveRequest() failed [method:%s]: %s', method, error.toString());

				throw error;
			});
	}

	receiveNotification(notification)
	{
		if (this.closed)
			return Promise.reject(new errors.InvalidStateError('Room closed'));
		else if (typeof notification !== 'object')
			return Promise.reject(new TypeError('wrong notification Object'));
		else if (notification.notification !== true)
			return Promise.reject(new TypeError('not a notification'));
		else if (typeof notification.method !== 'string')
			return Promise.reject(new TypeError('wrong/missing notification method'));

		const { method } = notification;

		logger.debug('receiveNotification() [method:%s]', method);

		return Promise.resolve()
			.then(() =>
			{
				switch (method)
				{
					// TODO: Handle appData.
					case 'leave':
					{
						this.close();

						break;
					}

					default:
						throw new Error(`unknown notification method "${method}"`);
				}
			})
			.catch((error) =>
			{
				logger.error(
					'receiveNotification() failed [method:%s]: %s', method, error.toString());
			});
	}

	_createTransport({ id, options, remoteDtlsParameters })
	{
		logger.debug('_createTransport()');

		const internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			transportId : id || utils.randomNumber()
		};

		let transport;

		return this._channel.request('peer.createTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"peer.createTransport" request succeeded');

				// Create a Transport instance.
				transport = new Transport(internal, data, this._channel);

				// Store the Transport instance and remove it when closed.
				this._transports.add(transport);
				transport.on('close', () => this._transports.delete(transport));

				this.emit('newtransport', transport);
			})
			.then(() =>
			{
				// If client's DTLS parameters are given, provide the Transport with
				// them.
				if (remoteDtlsParameters)
				{
					// TODO: The Transport should accept the client's given format.
					// Or may be we should choose here if many are given.
					remoteDtlsParameters.fingerprint =
						remoteDtlsParameters.fingerprints[0];

					transport.setRemoteDtlsParameters(remoteDtlsParameters)
						.catch((error) => transport.close(error));
				}

				return transport;
			})
			.catch((error) =>
			{
				logger.error('"peer.createTransport" request failed: %s', error);

				throw error;
			});
	}
}

module.exports = Peer;
