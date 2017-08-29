'use strict';

const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const errors = require('./errors');
const utils = require('./utils');
const ortc = require('./ortc');
const Peer = require('./Peer');

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
		// - .roomId
		this._internal = internal;

		// Room data.
		// - .rtpCapabilities
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// Map of Peer instances indexed by `peerName`.
		this._peers = new Map();

		// TODO: Yes?
		// Map of Producer instances indexed by `producerId`.
		this._producers = new Map();

		this._handleNotifications();
	}

	get id()
	{
		return this._internal.roomId;
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
		this._channel.removeAllListeners(this._internal.roomId);

		// Close every Peer.
		for (const peer of this._peers.values())
		{
			peer.close(undefined, false);
		}

		if (notifyChannel)
		{
			// Send Channel request.
			this._channel.request('room.close', this._internal)
				.then(() =>
				{
					logger.debug('"room.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"room.close" request failed: %s', String(error));
				});
		}
	}

	/**
	 * Dump the Room.
	 *
	 * @return {Promise}
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Room closed'));

		return this._channel.request('room.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"room.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"room.dump" request failed: %s', String(error));

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
						const { peerName, rtpCapabilities, appData } = request;

						// Ensure that the given capabilties are a subset of the Room
						// capabilities.
						// NOTE: This may throw.
						ortc.assertCapabilitiesSubset(
							rtpCapabilities, this.rtpCapabilities);

						const peer = this._createPeer(peerName, rtpCapabilities, appData);

						const response =
						{
							peers : [] // TODO: remove...
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

	_createPeer(peerName, rtpCapabilities, appData)
	{
		logger.debug('_createPeer() [peerName:"%s]', peerName);

		if (!peerName || typeof peerName !== 'string')
			return Promise.reject(new TypeError('peerName must be a string'));
		else if (this._peers.has(peerName))
			return Promise.reject(new Error(`duplicated Peer [peerName:"${peerName}"]`));

		const internal =
		{
			roomId   : this._internal.roomId,
			peerId   : utils.randomNumber(),
			peerName : peerName
		};

		const data =
		{
			rtpCapabilities
		};

		const sandbox =
		{
			getProducerById : (producerId) =>
			{
				return this._producers.get(producerId);
			},

			// NOTE: This may throw.
			getProducerRtpParametersMapping : (rtpParameters) =>
			{
				return ortc.getProducerRtpParametersMapping(
					rtpParameters, this.rtpCapabilities);
			}
		};

		// Create a Peer.
		const peer = new Peer(internal, data, this._channel, sandbox);

		peer.appData = appData;

		// Store the Peer and remove it when closed.
		this._peers.set(peer.name, peer);
		peer.on('@close', () => this._peers.delete(peer.name));

		// Listen for new Producers so we can associate new Consumers.
		peer.on('@newproducer', (producer) =>
		{
			// Store the Producer and remove it when closed.
			this._producers.set(producer.id, producer);
			producer.on('@close', () => this._producers.delete(producer.id));
		});

		this._channel.request('room.createPeer', internal)
			.then(() =>
			{
				logger.debug('"room.createPeer" request succeeded');

				peer.joined = true;
			})
			.catch((error) =>
			{
				logger.error('"room.createPeer" request failed: %s', String(error));

				peer.close();
			});

		this.safeEmit('newpeer', peer);

		return peer;
	}

	_handleNotifications()
	{
		// Subscribe to notifications.
		this._channel.on(this._internal.roomId, (event, data) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, false);

					break;
				}

				case 'audiolevels':
				{
					const entries = data.entries;
					const entries2 = [];

					for (const entry of entries)
					{
						const producerId = entry[0];
						const audioLevel = entry[1];
						const producer = this._producers.get(producerId);

						if (!producer)
							continue;

						const peer = producer.peer;

						entries2.push({ peer, producer, audioLevel });
					}

					const orderedEntries = entries2.sort((a, b) =>
					{
						if (a.audioLevel > b.audioLevel)
							return -1;
						else if (a.audioLevel < b.audioLevel)
							return 1;
						else
							return 0;
					});

					this.safeEmit('audiolevels', orderedEntries);

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

					// Send Channel request.
					this._channel.request(
						'room.setAudioLevelsEvent', this._internal, { enabled: true })
						.then(() =>
						{
							logger.debug('"room.setAudioLevelsEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error(
								'"room.setAudioLevelsEvent" request failed: %s', String(error));
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

					// Send Channel request.
					this._channel.request(
						'room.setAudioLevelsEvent', this._internal, { enabled: false })
						.then(() =>
						{
							logger.debug('"room.setAudioLevelsEvent" request succeeded');
						})
						.catch((error) =>
						{
							logger.error(
								'"room.setAudioLevelsEvent" request failed: %s', String(error));
						});

					break;
				}
			}
		});
	}
}

module.exports = Room;
