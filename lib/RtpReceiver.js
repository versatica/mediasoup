'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('RtpReceiver');
const errors = require('./errors');

const RTP_LISTEN_MODE_RAW = Symbol('raw');
const RTP_LISTEN_MODE_OBJECT = Symbol('object');

class RtpReceiver extends EventEmitter
{
	constructor(internal, data, channel)
	{
		logger.debug('constructor() [internal:%o, data:%o]', internal, data);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data.
		// - .roomId
		// - .peerId
		// - .rtpReceiverId
		// - .transportId
		this._internal = internal;

		// RtcReceiver data.
		// - .kind
		// - .transport
		// - .rtpParameters
		this._data = data;

		// Channel instance.
		this._channel = channel;

		// RTP listen mode.
		this._rtpListenMode = null;

		// Closed flag.
		this._closed = false;

		// Subscribe to notifications.
		this._channel.on(this._internal.rtpReceiverId, (event, data, buffer) =>
		{
			switch (event)
			{
				case 'close':
				{
					this.close(undefined, true);
					break;
				}

				case 'rtp':
				{
					switch (this._rtpListenMode)
					{
						case RTP_LISTEN_MODE_RAW:
							this.emit('rtp', buffer);
							break;
						case RTP_LISTEN_MODE_OBJECT:
							data.object.payload = buffer;
							this.emit('rtp', data.object);
							break;
					}
					break;
				}

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}

	get closed()
	{
		return this._closed;
	}

	get kind()
	{
		return this._data.kind;
	}

	get transport()
	{
		return this._data.transport;
	}

	get rtpParameters()
	{
		return this._data.rtpParameters;
	}

	get rtpListenMode()
	{
		switch (this._rtpListenMode)
		{
			case RTP_LISTEN_MODE_RAW:
				return 'raw';
			case RTP_LISTEN_MODE_OBJECT:
				return 'object';
			default:
				return undefined;
		}
	}

	set rtpListenMode(mode)
	{
		logger.debug('set rtpListenMode [mode:%s]', mode);

		if (this._closed)
			return;

		let data =
		{
			mode : mode
		};

		// Send Channel request.
		this._channel.request('rtpReceiver.rtpListenMode', this._internal, data)
			.then(() =>
			{
				logger.debug('"rtpReceiver.rtpListenMode" request succeeded');

				switch (mode)
				{
					case 'raw':
						this._rtpListenMode = RTP_LISTEN_MODE_RAW;
						break;
					case 'object':
						this._rtpListenMode = RTP_LISTEN_MODE_OBJECT;
						break;
					default:
						this._rtpListenMode = null;

				}
			})
			.catch((error) =>
			{
				logger.error('"rtpReceiver.rtpListenMode" request failed: %s', error);
			});
	}

	/**
	 * Close the RtpReceiver.
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

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.rtpReceiverId);

		if (!dontSendChannel)
		{
			// Send Channel request.
			this._channel.request('rtpReceiver.close', this._internal)
				.then(() =>
				{
					logger.debug('"rtpReceiver.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"rtpReceiver.close" request failed: %s', error);
				});
		}

		this.emit('close', error);
	}

	/**
	 * Dump the RtpReceiver.
	 * @return {Promise}  Resolves to object data.
	 */
	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpReceiver closed'));

		return this._channel.request('rtpReceiver.dump', this._internal)
			.then((data) =>
			{
				logger.debug('"rtpReceiver.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"rtpReceiver.dump" request failed: %s', error);

				throw error;
			});
	}

	isRtpReceiver()
	{
		return true;
	}

	isRtpSender()
	{
		return false;
	}

	/**
	 * Start receiving media.
	 * @param  {Object} rtpParameters.
	 * @return {Promise}  Resolves to this.
	 */
	receive(rtpParameters)
	{
		logger.debug('receive() [rtpParameters:%o]', rtpParameters);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('RtpReceiver closed'));

		// Send Channel request.
		return this._channel.request('rtpReceiver.receive', this._internal, rtpParameters)
			.then((data) =>
			{
				logger.debug('"rtpReceiver.receive" request succeeded');

				this._data.rtpParameters = data;

				return this;
			})
			.catch((error) =>
			{
				logger.error('"rtpReceiver.receive" request failed: %s', error);

				throw error;
			});
	}
}

module.exports = RtpReceiver;
