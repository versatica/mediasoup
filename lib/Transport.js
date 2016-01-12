'use strict';

const EventEmitter = require('events').EventEmitter;

const logger = require('./logger')('Transport');
const utils = require('./utils');
const errors = require('./errors');

class Transport extends EventEmitter
{
	constructor(internal, data, channel)
	{
		logger.debug('constructor() [internal:%o, data:%o]', internal, data);

		super();
		this.setMaxListeners(Infinity);

		// Store internal data
		// - .roomId
		// - .peerId
		// - .transportId
		this._internal = internal;

		// Transport data provided by the worker
		// - .iceRole
		// - .iceComponent
		// - .iceLocalParameters
		//   - .usernameFragment
		//   - .password
		// - .iceLocalCandidates []
		//   - .foundation
		//   - .priority
		//   - .ip
		//   - .port
		//   - .type
		//   - .protocol
		//   - .tcpType
		// - .iceSelectedTuple
		//   - .local
		//     - .ip
		//     - .port
		//     - .protocol
		//   - .remote
		//     - .ip
		//     - .port
		//     - .protocol
		// - .iceState
		// - .dtlsLocalParameters
		//   - .role
		//   - .fingerprints
		//     - .sha-1
		//     - .sha-224
		//     - .sha-256
		//     - .sha-384
		//     - .sha-512
		// - .dtlsState
		this._data = data;

		// Channel instance
		this._channel = channel;

		// Closed flag
		this._closed = false;

		// Subscribe to notifications
		this._channel.on(this._internal.transportId, (event, data) =>
		{
			switch (event)
			{
				case 'iceselectedtuplechange':
				{
					this._data.iceSelectedTuple = data.iceSelectedTuple;
					// Emit it to the app
					this.emit(event, data);
					break;
				}

				case 'icestatechange':
				{
					this._data.iceState = data.iceState;
					// Emit it to the app
					this.emit(event, data);
					break;
				}

				case 'dtlsstatechange':
				{
					this._data.dtlsState = data.dtlsState;
					// Emit it to the app
					this.emit(event, data);
					break;
				}

				case 'close':
					this.close(undefined, true);
					break;

				default:
					logger.error('ignoring unknown event "%s"', event);
			}
		});
	}

	get closed()
	{
		return this._closed;
	}

	get iceRole()
	{
		return this._data.iceRole;
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

	get iceSelectedTuple()
	{
		return this._data.iceSelectedTuple;
	}

	get iceState()
	{
		return this._data.iceState;
	}

	get dtlsLocalParameters()
	{
		return this._data.dtlsLocalParameters;
	}

	get dtlsState()
	{
		return this._data.dtlsState;
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

		// Remove notification subscriptions
		this._channel.removeAllListeners(this._internal.transportId);

		// We won't receive events anymore, so be nice and manually update some
		// fields
		delete this._data.iceSelectedTuple;
		this._data.iceState = 'closed';
		this._data.dtlsState = 'closed';

		if (!dontSendChannel)
		{
			let internal = this._internal;

			// Send Channel request
			this._channel.request('transport.close', internal)
				.then(() =>
				{
					logger.debug('"transport.close" request succeeded');
				})
				.catch((error) =>
				{
					logger.error('"transport.close" request failed: %s', error);
				});
		}

		this.emit('close', error);
	}

	dump()
	{
		logger.debug('dump()');

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Transport closed'));

		let internal = this._internal;

		return this._channel.request('transport.dump', internal)
			.then((data) =>
			{
				logger.debug('"transport.dump" request succeeded');

				return data;
			})
			.catch((error) =>
			{
				logger.error('"transport.dump" request failed: %s', error);

				throw error;
			});
	}

	/**
	 * Provide the remote DTLS parameters
	 * @param  {Object} options  Remote DTLS parameters
	 * @return {Promise}
	 */
	setRemoteDtlsParameters(options)
	{
		logger.debug('setRemoteDtlsParameters() [options:%o]', options);

		if (this._closed)
			return Promise.reject(new errors.InvalidStateError('Transport closed'));

		let internal = this._internal;

		// Send Channel request
		return this._channel.request('transport.setRemoteDtlsParameters', internal, options)
			.then((data) =>
			{
				logger.debug('"transport.setRemoteDtlsParameters" request succeeded');

				// Take the .role field of the response data and append it to our
				// this._data.dtlsLocalParameters
				this._data.dtlsLocalParameters.role = data.role;

				// Return the transport itself so the user can handle it in the next
				// `then()` level`
				return this;
			})
			.catch((error) =>
			{
				logger.error('"transport.setRemoteDtlsParameters" request failed: %s', error);

				throw error;
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
			return Promise.reject(new errors.InvalidStateError('Transport closed'));

		let internal =
		{
			roomId         : this._internal.roomId,
			peerId         : this._internal.peerId,
			transportId    : utils.randomNumber(),
			rtpTransportId : this._internal.transportId
		};

		return this._channel.request('peer.createAssociatedTransport', internal)
			.then((data) =>
			{
				logger.debug('"peer.createAssociatedTransport" request succeeded');

				// Create a Transport instance
				let transport = new Transport(internal, data, this._channel);

				// Emit 'associatedtransport' so the owner Peer can handle this new
				// transport
				this.emit('associatedtransport', transport);

				// Return the new transport
				return transport;
			})
			.catch((error) =>
			{
				logger.error('"transport.createAssociatedTransport" request failed: %s', error);

				throw error;
			});
	}
}

module.exports = Transport;
