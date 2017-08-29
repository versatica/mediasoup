'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const utils = require('./utils');
const errors = require('./errors');
const Transport = require('./Transport');
const Producer = require('./Producer');
const Consumer = require('./Consumer');

const KINDS = new Set([ 'audio', 'video', 'depth' ]);

const logger = new Logger('Peer');

class Peer extends EnhancedEventEmitter
{
	constructor(internal, data, channel, sandbox)
	{
		super(logger);

		logger.debug('constructor() [internal:%o]', internal);

		// Closed flag.
		this._closed = false;

		// Internal data.
		// - .roomId
		// - .peerId
		// - .peerName
		this._internal = internal;

		// Peer data.
		// - .rtpCapabilities
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Store internal sandbox stuff.
		// - .getProducerById()
		this._sandbox = sandbox;

		// App data.
		this._appData = undefined;

		// Whether we are joined so can emit '@notify' events.
		this._joined = false;

		// Map of Transports indexed by id.
		this._transports = new Map();

		// Map of Producers indexed by id.
		this._producers = new Map();

		// Map of Consumers indexed by id.
		this._consumers = new Map();

		this._handleNotifications();
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

	get appData()
	{
		return this._appData;
	}

	set appData(appData)
	{
		this._appData = appData;
	}

	set joined(flag)
	{
		this._joined = Boolean(joined);
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
		return Array.from(this._transports.values());
	}

	/**
	 * Get an array with all the Producers.
	 *
	 * @return {Array<Producers>}
	 */
	get producers()
	{
		return Array.from(this._producers.values());
	}

	/**
	 * Get an array with all the Consumer.
	 *
	 * @return {Array<Consumer>}
	 */
	get consumers()
	{
		return Array.from(this._consumers.values());
	}

	/**
	 * Close the Peer.
	 *
	 * @param {Any} [appData] - App custom data.
	 * @param {Boolean} [notifyChannel=true] - Private.
	 */
	close(appData, notifyChannel = true)
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		// Send a notification.
		this._sendNotification('closed', { appData });

		this.emit('@close');
		this.safeEmit('close', 'local', appData);

		// Close all the Producers.
		for (const producer of this._producers.values())
		{
			producer.close(undefined, false);
		}

		// Close all the Consumers.
		for (const consumer of this._consumers.values())
		{
			consumer.close();
		}

		// Close all the Transports.
		for (const transport of this._transports.values())
		{
			transport.close(undefined, false);
		}

		this._destroy(notifyChannel);
	}

	/**
	 * The remote Peer was closed.
	 * Invoked via remote notification.
	 *
	 * @private
	 *
	 * @param {Any} [appData] - App custom data.
	 */
	remoteClose(appData)
	{
		logger.debug('remoteClose()');

		if (this.closed)
			return;

		this._closed = true;

		this.emit('@close');
		this.safeEmit('close', 'remote', appData);

		// Close all the Producers.
		for (const producer of this._producers.values())
		{
			producer.remoteClose(undefined, false);
		}

		// Close all the Consumers.
		for (const consumer of this._consumers.values())
		{
			consumer.remoteClose();
		}

		// Close all the Transports.
		for (const transport of this._transports.values())
		{
			transport.remoteClose(undefined, false);
		}

		this._destroy();
	}

	/**
	 * Destroy the Peer.
	 */
	_destroy(notifyChannel = true)
	{
		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.peerId);

		if (notifyChannel)
		{
			// Send Channel request.
			this._channel.request('peer.close', this._internal)
				.then(() =>
				{
					logger.debug('"peer.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"peer.close" request failed: %s', String(error));
				});
		}
	}

	/**
	 * Get Transport by id.
	 *
	 * @param {Number} id
	 *
	 * @return {Transport}
	 */
	getTransportById(id)
	{
		return this._transports.get(id);
	}

	/**
	 * Get Producer by id.
	 *
	 * @param {Number} id
	 *
	 * @return {Producer}
	 */
	getProducerById(id)
	{
		return this._producers.get(id);
	}

	/**
	 * Get Consumer by id.
	 *
	 * @param {Number} id
	 *
	 * @return {Consumer}
	 */
	getConsumerById(id)
	{
		return this._consumers.get(id);
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
				logger.error('"peer.dump" request failed: %s', String(error));

				throw error;
			});
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
					case 'createTransport':
					{
						const { id, options, dtlsParameters, appData } = request;

						return this._createTransport(id, options)
							.then((transport) =>
							{
								transport.appData = appData;

								// If no client's DTLS parameters are given, we are done.
								if (!dtlsParameters)
									return transport;

								return transport.setRemoteDtlsParameters(dtlsParameters)
									.then(() => transport)
									.catch((error) =>
									{
										transport.remoteClose();

										throw error;
									});
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

					case 'restartTransport':
					{

						const { id } = request;
						const transport = this._transports.get(id);

						if (!transport)
							throw new Error('Transport not found');

						return transport.changeUfragPwd()
							.then(() =>
							{
								const response =
								{
									iceParameters : transport.iceLocalParameters
								};

								return response;
							});
					}

					case 'createProducer':
					{
						const {
							id,
							kind,
							transportId,
							rtpParameters,
							paused,
							appData
						} = request;

						// NOTE: This may throw.
						const rtpMapping =
							this._sandbox.getProducerRtpParametersMapping(rtpParameters);

						return this._createProducer(id, kind, transportId)
							.then((producer) =>
							{
								producer.appData = appData;

								if (paused)
									producer.remotePause();

								return producer.receive(rtpParameters, rtpMapping)
									.catch((error) =>
									{
										producer.remoteClose();

										throw error;
									});
							})
							.then(() =>
							{
								return;
							});
					}

					default:
						throw new Error(`unknown request method "${method}"`);
				}
			})
			.catch((error) =>
			{
				logger.error('receiveRequest() failed [method:%s]: %o', method, error);

				throw error;
			});
	}

	receiveNotification(notification)
	{
		if (this._closed)
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
					case 'leave':
					{
						const { appData } = notification;

						this.remoteClose(appData);

						break;
					}

					case 'updateTransport':
					{
						const { id, dtlsParameters } = notification;
						const transport = this._transports.get(id);

						if (!transport)
							throw new Error('Transport not found');

						return transport.setRemoteDtlsParameters(dtlsParameters)
							.catch((error) =>
							{
								transport.close();

								throw error;
							});
					}

					case 'closeTransport':
					{
						const { id, appData } = notification;
						const transport = this._transports.get(id);

						if (!transport)
							throw new Error('Transport not found');

						transport.remoteClose(appData);

						break;
					}

					// TODO: Do it.
					case 'updateProducer':
					{
						throw new Error('updateProducer not yet implemented');
					}

					case 'pauseProducer':
					{
						const { id, appData } = notification;
						const producer = this._producers.get(id);

						if (!producer)
							throw new Error('Producer not found');

						producer.remotePause(appData);

						break;
					}

					case 'resumeProducer':
					{
						const { id, appData } = notification;
						const producer = this._producers.get(id);

						if (!producer)
							throw new Error('Producer not found');

						producer.remoteResume(appData);

						break;
					}

					case 'closeProducer':
					{
						const { id, appData } = notification;
						const producer = this._producers.get(id);

						// Just ignore it.
						if (!producer)
							break;

						producer.remoteClose(appData);

						break;
					}

					default:
						throw new Error(`unknown notification method "${method}"`);
				}
			})
			.catch((error) =>
			{
				logger.error(
					'receiveNotification() failed [method:%s]: %o', method, error);
			});
	}

	_sendNotification(method, data)
	{
		// Ignore if closed.
		if (this._closed)
			return;

		const notification =
			Object.assign({ method, target: 'peer', notification: true }, data);

		logger.debug('_sendNotification() [method:%s]', method);

		this.safeEmit('notify', notification);
	}

	_createTransport(id, options)
	{
		logger.debug('_createTransport() [id:%s]', id);

		const internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			transportId : id || utils.randomNumber()
		};

		return this._channel.request('peer.createTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"peer.createTransport" request succeeded');

				// Create a Transport.
				const transport = new Transport(internal, data, this._channel);

				// Store the Transport and remove it when closed.
				this._transports.set(transport.id, transport);
				transport.on('@close', () => this._transports.delete(transport.id));

				transport.on('@notify', (method, data2) =>
				{
					this._sendNotification(method, data2);
				});

				this.safeEmit('newtransport', transport);

				return transport;
			})
			.catch((error) =>
			{
				logger.error('"peer.createTransport" request failed: %s', String(error));

				throw error;
			});
	}

	_createProducer(id, kind, transportId)
	{
		logger.debug('_createProducer() [id:%s, kind:%s]', id, kind);

		if (!this.rtpCapabilities)
			return Promise.reject(new errors.InvalidStateError('RTP capabilities unset'));
		else if (!KINDS.has(kind))
			return Promise.reject(new TypeError(`unsupported kind: ${kind}`));

		const transport = this._transports.get(transportId);

		if (!transport)
			return Promise.reject(new TypeError('Transport not found'));

		const internal =
		{
			roomId      : this._internal.roomId,
			peerId      : this._internal.peerId,
			producerId  : id || utils.randomNumber(),
			transportId : transport.id
		};

		const data =
		{
			kind      : kind,
			peer      : this,
			transport : transport
		};

		const options =
		{
			kind
		};

		return this._channel.request('peer.createProducer', internal, options)
			.then(() =>
			{
				logger.debug('"peer.createProducer" request succeeded');

				// Create a Producer instance.
				const producer = new Producer(internal, data, this._channel);

				// Store the Producer and remove it when closed.
				this._producers.set(producer.id, producer);
				producer.on('@close', () => this._producers.delete(producer.id));

				producer.on('@notify', (method, data2) =>
				{
					this._sendNotification(method, data2);
				});

				// On Transport closure close the Producer.
				transport.on('@close', () =>
				{
					if (!producer.closed)
						producer.close();
				});

				this.emit('@newproducer', producer);
				this.safeEmit('newproducer', producer);

				return producer;
			})
			.catch((error) =>
			{
				logger.error('"peer.createProducer" request failed: %s', String(error));

				throw error;
			});
	}

	_handleNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.peerId, (event, data) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, false);

					break;
				}

				// TODO: yes? or get it from data.peerName in 'newconsumer' and retreive
				// peers via JS? Then what about `peerClosed`?
				case 'newpeer':
				{
					// TODO

					break;
				}

				case 'newconsumer':
				{
					// TODO
					console.warn('----------------- newconsumer | peer.consumers:%s', this._consumers.size + 1);

					// TODO
					const source = this._sandbox.getProducerById(data.sourceId);

					const consumerInternal =
					{
						roomId      : this._internal.roomId,
						peerId      : this._internal.peerId,
						consumerId  : data.consumerId,
						transportId : undefined
					};

					// TODO
					const consumerData =
					{
						kind          : data.kind,
						peer          : this,
						rtpParameters : data.rtpParameters,
						source        : source
					};

					// Create a Consumer instance.
					const consumer =
						new Consumer(consumerInternal, consumerData, this._channel);

					// Store the Consumer instance and remove it when closed.
					this._consumers.set(consumer.id, consumer);
					consumer.on('@close', () => this._consumers.delete(consumer.id));

					// TODO: Get the Consumer Peer?

					if (this._joined)
					{
						// TODO: Let's see.
						// TODO: appData? of the associated Producer/source?
						// const foo = consumer.toJson();
						const foo = { id: consumer.id, kind: consumer.kind };

						// Send a notification.
						this._sendNotification('newConsumer', foo);
					}

					this.safeEmit('newconsumer', consumer);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}
}

module.exports = Peer;
