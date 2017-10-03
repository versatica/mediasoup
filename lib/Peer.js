'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const utils = require('./utils');
const errors = require('./errors');
const ortc = require('./ortc');
const WebRtcTransport = require('./WebRtcTransport');
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
		// - .routerId
		// - .peerName
		this._internal = internal;

		// Peer data.
		// - .rtpCapabilities
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Store internal sandbox stuff.
		// - .getProducerRtpParametersMapping()
		this._sandbox = sandbox;

		// App data.
		this._appData = undefined;

		// Map of WebRtcTransports indexed by id.
		// @type {Map<transportId, WebRtcTransport>}
		this._transports = new Map();

		// Map of Producers indexed by id.
		// @type {Map<producerId, Producer>}
		this._producers = new Map();

		// Map of Consumers indexed by id.
		// @type {Map<consumerId, Consumers>}
		this._consumers = new Map();
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

	get rtpCapabilities()
	{
		return this._data.rtpCapabilities;
	}

	/**
	 * Get an array with all the WebRtcTransports.
	 *
	 * @return {Array<WebRtcTransport>}
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

		// Send a notification.
		this._sendNotification('closed', { appData });

		this._closed = true;

		this.emit('@close', appData);
		this.safeEmit('close', 'local', appData);

		// Close all the Producers.
		for (const producer of this._producers.values())
		{
			producer.close(undefined, notifyChannel);
		}

		// Close all the Consumers.
		for (const consumer of this._consumers.values())
		{
			consumer.close(undefined, notifyChannel);
		}

		// Close all the WebRtcTransports.
		for (const transport of this._transports.values())
		{
			transport.close(undefined, notifyChannel);
		}
	}

	/**
	 * The remote Peer left the Room.
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

		this.emit('@close', appData);
		this.safeEmit('close', 'remote', appData);

		// Close all the Producers.
		for (const producer of this._producers.values())
		{
			producer.remoteClose();
		}

		// Close all the Consumers.
		for (const consumer of this._consumers.values())
		{
			consumer.close();
		}

		// Close all the WebRtcTransports.
		for (const transport of this._transports.values())
		{
			transport.remoteClose();
		}
	}

	/**
	 * Get WebRtcTransport by id.
	 *
	 * @param {Number} id
	 *
	 * @return {WebRtcTransport}
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

		const peerData =
		{
			peerName   : this.name,
			transports : [],
			producers  : [],
			consumers  : []
		};

		const promises = [];

		for (const transport of this._transports.values())
		{
			const promise = transport.dump()
				.then((data) =>
				{
					peerData.transports.push(data);
				});

			promises.push(promise);
		}

		for (const producer of this._producers.values())
		{
			const promise = producer.dump()
				.then((data) =>
				{
					peerData.producers.push(data);
				});

			promises.push(promise);
		}

		for (const consumer of this._consumers.values())
		{
			const promise = consumer.dump()
				.then((data) =>
				{
					peerData.consumers.push(data);
				});

			promises.push(promise);
		}

		return Promise.all(promises)
			.then(() =>
			{
				logger.debug('dump() | succeeded');

				return peerData;
			})
			.catch((error) =>
			{
				logger.error('dump() | failed: %s', String(error));

				throw error;
			});
	}

	toJson()
	{
		return {
			name    : this.name,
			appData : this.appData
		};
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
						const {
							id,
							direction,
							options,
							dtlsParameters,
							appData
						} = request;

						return this._createWebRtcTransport(id, direction, options, appData)
							.then((transport) =>
							{
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

						const remotelyPaused = paused;

						return this._createProducer(
							id, kind, transportId, rtpParameters, remotelyPaused, appData);
					}

					case 'enableConsumer':
					{
						const { id, transportId, paused, preferredProfile } = request;
						const consumer = this._consumers.get(id);
						const transport = this._transports.get(transportId);

						if (!consumer)
							throw new Error('Consumer not found');
						else if (!transport)
							throw new TypeError('Transport not found');

						return consumer.enable(transport, paused, preferredProfile)
							.then(() =>
							{
								return {
									paused           : consumer.sourcePaused || consumer.locallyPaused,
									preferredProfile : consumer.preferredProfile,
									effectiveProfile : consumer.effectiveProfile
								};
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

					case 'updateProducer':
					{
						const { id, rtpParameters } = notification;
						const producer = this._producers.get(id);

						if (!producer)
							throw new Error('Producer not found');

						producer.updateRtpParameters(rtpParameters);

						break;
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

					case 'pauseConsumer':
					{
						const { id, appData } = notification;
						const consumer = this._consumers.get(id);

						if (!consumer)
							throw new Error('Consumer not found');

						consumer.remotePause(appData);

						break;
					}

					case 'resumeConsumer':
					{
						const { id, appData } = notification;
						const consumer = this._consumers.get(id);

						if (!consumer)
							throw new Error('Consumer not found');

						consumer.remoteResume(appData);

						break;
					}

					case 'setConsumerPreferredProfile':
					{
						const { id, profile } = notification;
						const consumer = this._consumers.get(id);

						if (!consumer)
							throw new Error('Consumer not found');

						consumer.remoteSetPreferredProfile(profile);

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

	/**
	 * @private
	 */
	handleNewPeer(peer)
	{
		const peerData = Object.assign({ consumers: [] }, peer.toJson());

		this._sendNotification('newPeer', peerData);
	}

	/**
	 * @private
	 */
	handlePeerClosed(peer, appData)
	{
		this._sendNotification('peerClosed', { name: peer.name, appData });
	}

	/**
	 * @private
	 */
	addConsumerForProducer(producer, notifyClient = true)
	{
		const internal =
		{
			routerId    : this._internal.routerId,
			consumerId  : utils.randomNumber(),
			producerId  : producer.id,
			transportId : undefined
		};

		const consumerRtpParameters =
			ortc.getConsumerRtpParameters(
				producer.consumableRtpParameters, this.rtpCapabilities);

		const data =
		{
			kind          : producer.kind,
			peer          : producer.peer,
			transport     : undefined,
			rtpParameters : consumerRtpParameters,
			source        : producer
		};

		// Create a Consumer instance.
		const consumer = new Consumer(internal, data, this._channel);

		consumer.appData = producer.appData;

		// Store the Consumer and remove it when closed.
		this._consumers.set(consumer.id, consumer);
		consumer.on('@close', () => this._consumers.delete(consumer.id));

		consumer.on('@notify', (method, data2) =>
		{
			this._sendNotification(method, data2);
		});

		this._channel.request('router.createConsumer', internal,
			{
				kind : consumer.kind
			})
			.then(() =>
			{
				logger.debug('"router.createConsumer" request succeeded');
			})
			.catch((error) =>
			{
				logger.error('"router.createConsumer" request failed: %s', String(error));

				throw error;
			});

		this.safeEmit('newconsumer', consumer);

		if (notifyClient)
			this._sendNotification('newConsumer', consumer.toJson());
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

	_createWebRtcTransport(id, direction, options, appData)
	{
		logger.debug('_createWebRtcTransport() [id:%s, direction:%s]', id, direction);

		const internal =
		{
			routerId    : this._internal.routerId,
			transportId : id || utils.randomNumber()
		};

		return this._channel.request('router.createWebRtcTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"router.createWebRtcTransport" request succeeded');

				data = Object.assign(data, { direction });

				// Create a WebRtcTransport.
				const transport = new WebRtcTransport(internal, data, this._channel);

				transport.appData = appData;

				// Store the WebRtcTransport and remove it when closed.
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
				logger.error(
					'"router.createWebRtcTransport" request failed: %s', String(error));

				throw error;
			});
	}

	_createProducer(id, kind, transportId, rtpParameters, remotelyPaused, appData)
	{
		logger.debug('_createProducer() [id:%s, kind:%s]', id, kind);

		if (!this.rtpCapabilities)
			return Promise.reject(new errors.InvalidStateError('RTP capabilities unset'));
		else if (!KINDS.has(kind))
			return Promise.reject(new TypeError(`unsupported kind: ${kind}`));

		let rtpMappingForWorker;
		let consumableRtpParameters;

		try
		{
			const rtpMapping =
				this._sandbox.getProducerRtpParametersMapping(rtpParameters);

			// NOTE: rtpMapping has two Maps, but we need to convert them into arrays
			// of pairs for the worker.
			rtpMappingForWorker =
			{
				codecPayloadTypes  : Array.from(rtpMapping.mapCodecPayloadTypes),
				headerExtensionIds : Array.from(rtpMapping.mapHeaderExtensionIds)
			};

			consumableRtpParameters =
				this._sandbox.getConsumableRtpParameters(rtpParameters, rtpMapping);
		}
		catch (error)
		{
			return Promise.reject(new Error(`wrong RTP parameters: ${error}`));
		}

		const transport = this._transports.get(transportId);

		if (!transport)
			return Promise.reject(new TypeError('WebRtcTransport not found'));

		const internal =
		{
			routerId    : this._internal.routerId,
			producerId  : id || utils.randomNumber(),
			transportId : transport.id
		};

		const data =
		{
			kind                    : kind,
			peer                    : this,
			transport               : transport,
			rtpParameters           : rtpParameters,
			consumableRtpParameters : consumableRtpParameters
		};

		const options =
		{
			remotelyPaused
		};

		return this._channel.request('router.createProducer', internal,
			{
				kind          : kind,
				rtpParameters : rtpParameters,
				rtpMapping    : rtpMappingForWorker,
				paused        : remotelyPaused
			})
			.then(() =>
			{
				logger.debug('"router.createProducer" request succeeded');

				const producer = new Producer(internal, data, this._channel, options);

				producer.appData = appData;

				// Store the Producer and remove it when closed.
				this._producers.set(producer.id, producer);
				producer.on('@close', () => this._producers.delete(producer.id));

				producer.on('@notify', (method, data2) =>
				{
					this._sendNotification(method, data2);
				});

				this.emit('@newproducer', producer);
				this.safeEmit('newproducer', producer);

				// Ensure we return nothing.
				return;
			})
			.catch((error) =>
			{
				logger.error('"router.createProducer" request failed: %s', String(error));

				throw error;
			});
	}
}

module.exports = Peer;
