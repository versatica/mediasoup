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
		// - .getPeerByName()
		// - .getProducerById()
		this._sandbox = sandbox;

		// Peer data.
		// - .capabilities
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

	get capabilities()
	{
		return this._data.capabilities;
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

		// Close every Transport.
		for (const transport of this._transports)
		{
			transport.close(undefined, true);
		}

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
	 * Set peer capabilities.
	 *
	 * @param {RtpCapabilities} [capabilities]
	 *
	 * @return {Promise} Resolves to the effective capabilities.
	 */
	setCapabilities(capabilities)
	{
		logger.debug('setCapabilities() [capabilities:%o]', capabilities);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		return this._channel.request(
			'peer.setCapabilities', this._internal, capabilities)
			.then((effectiveCapabilities) =>
			{
				logger.debug('"peer.setCapabilities" request succeeded');

				// Set peer's effective capabilities.
				this._data.capabilities = effectiveCapabilities;

				this.emit('capabilities', effectiveCapabilities);

				return effectiveCapabilities;
			})
			.catch((error) =>
			{
				logger.error('"peer.setCapabilities" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Create a Transport instance.
	 *
	 * @param {[Object]} [options]
	 * @param {[Boolean]} [options.udp=true] - Listen UDP.
	 * @param {[Boolean]} [options.tcp=true] - Listen TCP.
	 * @param {[Boolean]} [options.preferIPv4=false] - Prefer IPv4 candidates.
	 * @param {[Boolean]} [options.preferIPv6=false] - Prefer IPv6 candidates.
	 * @param {[Boolean]} [options.preferUdp=false] - Prefer UDP candidates.
	 * @param {[Boolean]} [options.preferTcp=false] - Prefer TCP candidates.
	 *
	 * @return {Promise} Resolves to the created Transport.
	 */
	createTransport(options)
	{
		logger.debug('createTransport() [options:%o]', options);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Peer closed'));

		const internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			transportId : utils.randomNumber()
		};

		return this._channel.request('peer.createTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"peer.createTransport" request succeeded');

				// Create a Transport instance.
				const transport = new Transport(internal, data, this._channel);

				// Store the Transport instance and remove it when closed.
				this._transports.add(transport);
				transport.on('close', () => this._transports.delete(transport));

				this.emit('newtransport', transport);

				return transport;
			})
			.catch((error) =>
			{
				logger.error('"peer.createTransport" request failed: %s', error);

				throw error;
			});
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

		// Ensure `kind` is 'audio' / 'video' / 'depth'.
		if (KINDS.indexOf(kind) === -1)
			throw new TypeError(`unsupported kind: ${kind}`);

		// Ensure `transport` is given.
		if (!transport)
			throw new TypeError('transport not given');

		// Ensure `transport` is a Transport.
		if (!(transport instanceof Transport))
			throw new TypeError('transport must be a instance of Transport');

		// Ensure `transport` is not closed.
		if (transport.closed)
			throw new errors.InvalidStateError('transport is closed');

		// Ensure `transport` belongs to this Peer.
		if (!this._transports.has(transport))
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
}

module.exports = Peer;
