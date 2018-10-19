'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const utils = require('./utils');
const errors = require('./errors');
const ortc = require('./ortc');
const Peer = require('./Peer');
const Consumer = require('./Consumer');
const PlainRtpTransport = require('./PlainRtpTransport');
const plugins = require('./plugins');

const logger = new Logger('Room');

class Room extends EnhancedEventEmitter
{
	constructor(internal, data, channel)
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		this._closed = false;

		// Internal data.
		// - .routerId
		this._internal = internal;

		// Room data.
		// - .rtpCapabilities
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Map of Peer instances indexed by peerName.
		// @type {Map<peerName, Peer>}
		this._peers = new Map();

		// Map of Producer instances indexed by producerId.
		// @type {Map<producerId, Producer>}
		this._producers = new Map();

		this._handleWorkerNotifications();
	}

	get id()
	{
		return this._internal.routerId;
	}

	get closed()
	{
		return this._closed;
	}

	get rtpCapabilities()
	{
		return this._data.rtpCapabilities;
	}

	/**
	 * Get an array with all the Peers.
	 *
	 * @return {Array<Peer>}
	 */
	get peers()
	{
		return Array.from(this._peers.values());
	}

	/**
	 * Close the Room.
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

		this.emit('@close');
		this.safeEmit('close', appData);

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.routerId);

		// Close every Peer.
		for (const peer of this._peers.values())
		{
			peer.close(undefined, false);
		}

		if (notifyChannel)
		{
			this._channel.request('router.close', this._internal)
				.then(() =>
				{
					logger.debug('"router.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"router.close" request failed: %s', String(error));
				});
		}
	}

	/**
	 * Dump the Room.
	 *
	 * @private
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Room closed'));

		// TODO: Should we aggregate Peers info?

		return this._channel.request('router.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"router.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"router.dump" request failed: %s', String(error));

				throw error;
			});
	}

	/**
	 * Get Peer by name.
	 *
	 * @param {String} name
	 *
	 * @return {Peer}
	 */
	getPeerByName(name)
	{
		return this._peers.get(name);
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
					case 'queryRoom':
					{
						const response =
						{
							rtpCapabilities            : this.rtpCapabilities,
							mandatoryCodecPayloadTypes : [] // TODO
						};

						return response;
					}

					case 'join':
					{
						const { peerName, rtpCapabilities, spy, appData } = request;
						const peer = this._createPeer(peerName, rtpCapabilities, spy, appData);
						const response =
						{
							peers : this.peers
								.filter((otherPeer) => otherPeer !== peer && !otherPeer.spy)
								.map((otherPeer) =>
								{
									const consumers = peer.consumers
										.filter((consumer) => consumer.peer === otherPeer)
										.map((consumer) => consumer.toJson());

									return Object.assign({ consumers }, otherPeer.toJson());
								})
						};

						return response;
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

	createActiveSpeakerDetector()
	{
		logger.debug('createActiveSpeakerDetector()');

		const activeSpeakerDetector = new plugins.ActiveSpeakerDetector(this);

		return activeSpeakerDetector;
	}

	createRtpStreamer(producer, options)
	{
		logger.debug('createRtpStreamer()');

		if (!this._producers.has(producer.id))
			return Promise.reject(new Error('Producer not found'));

		let plainRtpTransport;
		let consumer;

		return Promise.resolve()
			// Create a PlainRtpTransport in the worker.
			.then(() =>
			{
				return this._createPlainRtpTransport(options)
					.then((transport) =>
					{
						plainRtpTransport = transport;
					});
			})
			// Create a Consumer.
			.then(() =>
			{
				const internal =
				{
					routerId    : this._internal.routerId,
					consumerId  : utils.randomNumber(),
					producerId  : producer.id,
					transportId : undefined
				};

				// TODO: The app should provide his own RTP capabilities.
				const consumerRtpParameters =
					ortc.getConsumerRtpParameters(
						producer.consumableRtpParameters, this.rtpCapabilities);

				const data =
				{
					kind          : producer.kind,
					transport     : undefined,
					rtpParameters : consumerRtpParameters,
					source        : producer
				};

				// Create a Consumer instance.
				consumer = new Consumer(internal, data, this._channel);

				return this._channel.request('router.createConsumer', internal,
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
			})
			.then(() =>
			{
				return consumer.enable(plainRtpTransport);
			})
			.then(() =>
			{
				const rtpStreamer = new plugins.RtpStreamer(consumer, plainRtpTransport);

				return rtpStreamer;
			})
			.catch((error) =>
			{
				if (plainRtpTransport)
					plainRtpTransport.close();

				if (consumer)
					consumer.close();

				throw error;
			});
	}

	_createPeer(peerName, rtpCapabilities, spy, appData)
	{
		logger.debug('_createPeer() [peerName:"%s]', peerName);

		if (!peerName || typeof peerName !== 'string')
			throw new TypeError('peerName must be a string');
		else if (this._peers.has(peerName))
			throw new Error(`duplicated Peer [peerName:"${peerName}"]`);

		// Ensure that the given capabilties are a subset of the Room capabilities.
		// NOTE: This may throw.
		ortc.assertCapabilitiesSubset(rtpCapabilities, this.rtpCapabilities);

		const internal =
		{
			routerId : this._internal.routerId,
			peerName : peerName
		};

		const data =
		{
			rtpCapabilities,
			spy : Boolean(spy)
		};

		const sandbox =
		{
			// NOTE: This may throw.
			getProducerRtpParametersMapping : (producerRtpParameters) =>
			{
				return ortc.getProducerRtpParametersMapping(
					producerRtpParameters, this.rtpCapabilities);
			},

			// NOTE: This may throw.
			getConsumableRtpParameters : (producerRtpParameters, rtpMapping) =>
			{
				return ortc.getConsumableRtpParameters(
					producerRtpParameters, this.rtpCapabilities, rtpMapping);
			}
		};

		// Create a Peer.
		const peer = new Peer(internal, data, this._channel, sandbox);

		peer.appData = appData;

		// Store the Peer and remove it when closed. Also notify all other Peers
		// (unless it's a spy peer).
		this._peers.set(peer.name, peer);
		peer.on('@close', (appData2) =>
		{
			this._peers.delete(peer.name);

			if (!spy)
			{
				for (const otherPeer of this._peers.values())
				{
					if (otherPeer === peer)
						continue;

					otherPeer.handlePeerClosed(peer, appData2);
				}
			}
		});

		// Create a Consumer for this Peer associated to each Producer in other
		// Peers.
		for (const otherPeer of this._peers.values())
		{
			if (otherPeer === peer)
				continue;

			for (const producer of otherPeer.producers)
			{
				peer.addConsumerForProducer(producer, false);
			}
		}

		// Listen for new Producers so we can create new Consumers for other Peers.
		peer.on('@newproducer', (producer) =>
		{
			this._handleProducer(producer);
		});

		// Tell all the Peers (but us) about the new Peer (unless this is a spy peer).
		if (!spy)
		{
			for (const otherPeer of this._peers.values())
			{
				if (otherPeer === peer)
					continue;

				otherPeer.handleNewPeer(peer);
			}
		}

		this.safeEmit('newpeer', peer);

		return peer;
	}

	_handleProducer(producer)
	{
		// Store the Producer and remove it when closed.
		this._producers.set(producer.id, producer);
		producer.on('@close', () => this._producers.delete(producer.id));

		// Tell all the Peers (but us) about the new Producer.
		for (const otherPeer of this._peers.values())
		{
			if (otherPeer === producer.peer)
				continue;

			otherPeer.addConsumerForProducer(producer);
		}
	}

	_createPlainRtpTransport(options)
	{
		logger.debug('_createPlainRtpTransport()');

		const internal =
		{
			routerId    : this._internal.routerId,
			transportId : utils.randomNumber()
		};

		return this._channel.request('router.createPlainRtpTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"router.createPlainRtpTransport" request succeeded');

				// Create a PlainRtpTransport.
				const transport = new PlainRtpTransport(internal, data, this._channel);

				return transport;
			})
			.catch((error) =>
			{
				logger.error(
					'"router.createPlainRtpTransport" request failed: %s', String(error));

				throw error;
			});
	}

	_handleWorkerNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.routerId, (event, data) =>
		{
			switch (event)
			{
				case 'audiolevels':
				{
					const { entries } = data;
					const levels = entries
						.map((entry) =>
						{
							return {
								producer   : this._producers.get(entry[0]),
								audioLevel : entry[1]
							};
						})
						.filter((entry) => Boolean(entry.producer))
						.sort((a, b) =>
						{
							if (a.audioLevel > b.audioLevel)
								return -1;
							else if (a.audioLevel < b.audioLevel)
								return 1;
							else
								return 0;
						});

					this.safeEmit('audiolevels', levels);

					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});

		// Subscribe to new events.
		this.on('newListener', (event) =>
		{
			switch (event)
			{
				case 'audiolevels':
				{
					// Ignore if there are listeners already.
					if (this.listenerCount('audiolevels'))
						return;

					this._channel.request(
						'router.setAudioLevelsEvent', this._internal, { enabled: true })
						.then(() =>
						{
							logger.debug('"router.setAudioLevelsEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error(
								'"router.setAudioLevelsEvent" request failed: %s', String(error));
						});

					break;
				}
			}
		});

		// Subscribe to events removal.
		this.on('removeListener', (event) =>
		{
			switch (event)
			{
				case 'audiolevels':
				{
					// Ignore if there are other remaining listeners.
					if (this.listenerCount('audiolevels'))
						return;

					this._channel.request(
						'router.setAudioLevelsEvent', this._internal, { enabled: false })
						.then(() =>
						{
							logger.debug('"router.setAudioLevelsEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error(
								'"router.setAudioLevelsEvent" request failed: %s', String(error));
						});

					break;
				}
			}
		});
	}
}

module.exports = Room;
