const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const { InvalidStateError } = require('./errors');
// const ortc = require('./ortc');
// const WebRtcTransport = require('./WebRtcTransport');
// const PlainRtpTransport = require('./PlainRtpTransport');

const logger = new Logger('Router');

class Router extends EnhancedEventEmitter
{
	/**
	 * @private
	 *
	 * @emits workerclose
	 * @emits @close
	 */
	constructor({ internal, data, channel })
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Internal data.
		// @type {Object}
		// - .routerId
		this._internal = internal;

		// Router data.
		// @type {Object}
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
	 * @returns {Array<Peer>}
	 */
	get peers()
	{
		return Array.from(this._peers.values());
	}

	/**
	 * Close the Router.
	 */
	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.routerId);

		this._channel.request('router.close', this._internal)
			.catch(() => {});

		this.emit('@close');

		// Close every Peer.
		// for (const peer of this._peers.values())
		// {
		// 	peer.close(undefined, false);
		// }

		// Close all the Transports.
		// for (const transport of this._transports.values())
		// {
		// 	transport.transportClosed();
		// }
		// this._transports.clear();
	}

	/**
	 * Worker was closed.
	 *
	 * @private
	 */
	workerClosed()
	{
		if (this._closed)
			return;

		logger.debug('workerClosed()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.routerId);

		// Close every Peer.
		// for (const peer of this._peers.values())
		// {
		// 	peer.close(undefined, false);
		// }

		// Close all the Transports.
		// for (const transport of this._transports.values())
		// {
		// 	transport.transportClosed();
		// }
		// this._transports.clear();

		this.safeEmit('workerclose');
	}

	/**
	 * Dump the Router.
	 *
	 * @private
	 *
	 * @returns {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new InvalidStateError('closed'));

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
	 * @returns {Peer}
	 */
	getPeerByName(name)
	{
		return this._peers.get(name);
	}

	receiveRequest(request)
	{
		if (this._closed)
			return Promise.reject(new InvalidStateError('Router closed'));
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
					case 'queryRouter':
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
			transportId : uuidv4()
		};

		return this._channel.request(
			'router.createPlainRtpTransport', internal, options)
			.then((data) =>
			{
				logger.debug('"router.createPlainRtpTransport" request succeeded');

				// Create a PlainRtpTransport.
				// const transport = new PlainRtpTransport(internal, data, this._channel);

				// return transport;
				return data; // TODO hehe
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
				{
					logger.error('ignoring unknown event "%s"', event);
				}
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

module.exports = Router;
