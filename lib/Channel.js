const netstring = require('netstring');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const { InvalidStateError } = require('./errors');

// netstring length for a 65536 bytes payload.
const NS_MESSAGE_MAX_LEN = 65543;
const NS_PAYLOAD_MAX_LEN = 65536;

class Channel extends EnhancedEventEmitter
{
	/**
	 * @private
	 */
	constructor({ socket, pid })
	{
		const logger = new Logger(`Channel[pid:${pid}]`);
		const workerLogger = new Logger(`worker[pid:${pid}]`);

		super(logger);

		this._logger = logger;
		this._workerLogger = workerLogger;

		this._logger.debug('constructor()');

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Unix Socket instance.
		// @type {net.Socket}
		this._socket = socket;

		// Next request id.
		// @type {Number}
		this._nextId = 0;

		// Map of pending sent request objects indexed by request id.
		// @type {Map<String, Object}
		this._sents = new Map();

		// Buffer for incomplete data received from the Channel's socket.
		// @type {Buffer}
		this._recvBuffer = null;

		// Read Channel responses/notifications from the worker.
		this._socket.on('data', (buffer) =>
		{
			if (!this._recvBuffer)
			{
				this._recvBuffer = buffer;
			}
			else
			{
				this._recvBuffer = Buffer.concat(
					[ this._recvBuffer, buffer ],
					this._recvBuffer.length + buffer.length);

				if (this._recvBuffer.length > NS_PAYLOAD_MAX_LEN)
				{
					this._logger.error('receiving buffer is full, discarding all data into it');

					// Reset the buffer and exit.
					this._recvBuffer = null;

					return;
				}
			}

			while (true) // eslint-disable-line no-constant-condition
			{
				let nsPayload;

				try
				{
					nsPayload = netstring.nsPayload(this._recvBuffer);
				}
				catch (error)
				{
					this._logger.error(
						'invalid data received from the worker process: %s', String(error));

					// Reset the buffer and exit.
					this._recvBuffer = null;

					return;
				}

				// Incomplete netstring message.
				if (nsPayload === -1)
					return;

				try
				{
					// We can receive JSON messages (Channel messages) or log strings.
					switch (nsPayload[0])
					{
						// 123 = '{' (a Channel JSON messsage).
						case 123:
							this._processMessage(JSON.parse(nsPayload));
							break;

						// 68 = 'D' (a debug log).
						case 68:
							this._workerLogger.debug(nsPayload.toString('utf8', 1));
							break;

						// 87 = 'W' (a warn log).
						case 87:
							this._workerLogger.warn(nsPayload.toString('utf8', 1));
							break;

						// 69 = 'E' (an error log).
						case 69:
							this._workerLogger.error(nsPayload.toString('utf8', 1));
							break;

						// 88 = 'X' (a dump log).
						case 88:
							// eslint-disable-next-line no-console
							console.log(nsPayload.toString('utf8', 1));
							break;

						default:
							// eslint-disable-next-line no-console
							console.warn(
								`worker[pid:${pid}] unexpected data: %s`, nsPayload.toString('utf8'));
					}
				}
				catch (error)
				{
					this._logger.error(
						'received invalid message from the worker process: %s', String(error));
				}

				// Remove the read payload from the buffer.
				this._recvBuffer =
					this._recvBuffer.slice(netstring.nsLength(this._recvBuffer));

				if (!this._recvBuffer.length)
				{
					this._recvBuffer = null;

					return;
				}
			}
		});

		this._socket.on('end', () => this._logger.debug('Channel ended by the worker process'));
		this._socket.on('error', (error) => this._logger.error('Channel error: %s', String(error)));
	}

	/**
	 * @private
	 */
	close()
	{
		if (this._closed)
			return;

		this._logger.debug('close()');

		this._closed = true;

		// Close every pending sent.
		for (const sent of this._sents.values())
		{
			sent.close();
		}

		// Remove event listeners but leave a fake 'error' hander to avoid
		// propagation.
		this._socket.removeAllListeners('end');
		this._socket.removeAllListeners('error');
		this._socket.on('error', () => {});

		// Destroy the socket after a while to allow pending incoming messages.
		setTimeout(() =>
		{
			try { this._socket.destroy(); }
			catch (error) {}
		}, 200);
	}

	/**
	 * @private
	 *
	 * @async
	 */
	async request(method, internal, data)
	{
		this._nextId < 4294967295 ? ++this._nextId : (this._nextId = 1);

		const id = this._nextId;

		this._logger.debug('request() [method:%s, id:%s]', method, id);

		if (this._closed)
			throw new InvalidStateError('Channel closed');

		const request = { id, method, internal, data };
		const ns = netstring.nsWrite(JSON.stringify(request));

		if (Buffer.byteLength(ns) > NS_MESSAGE_MAX_LEN)
			throw new Error('Channel request too big');

		// This may throw if closed or remote side ended.
		this._socket.write(ns);

		return new Promise((pResolve, pReject) =>
		{
			const timeout = 1000 * (15 + (0.1 * this._sents.size));
			const sent =
			{
				id      : id,
				method  : method,
				resolve : (data2) =>
				{
					if (!this._sents.delete(id))
						return;

					clearTimeout(sent.timer);
					pResolve(data2);
				},
				reject : (error) =>
				{
					if (!this._sents.delete(id))
						return;

					clearTimeout(sent.timer);
					pReject(error);
				},
				timer : setTimeout(() =>
				{
					if (!this._sents.delete(id))
						return;

					pReject(new Error('Channel request timeout'));
				}, timeout),
				close : () =>
				{
					clearTimeout(sent.timer);
					pReject(new InvalidStateError('Channel closed'));
				}
			};

			// Add sent stuff to the map.
			this._sents.set(id, sent);
		});
	}

	/**
	 * @private
	 */
	_processMessage(msg)
	{
		// If a response retrieve its associated request.
		if (msg.id)
		{
			const sent = this._sents.get(msg.id);

			if (!sent)
			{
				this._logger.error(
					'received response does not match any sent request [id:%s]', msg.id);

				return;
			}

			if (msg.accepted)
			{
				this._logger.debug(
					'request succeeded [method:%s, id:%s]', sent.method, sent.id);

				sent.resolve(msg.data);
			}
			else if (msg.error)
			{
				this._logger.warn(
					'request failed [method:%s, id:%s]: %s',
					sent.method, sent.id, msg.reason);

				switch (msg.error)
				{
					case 'TypeError':
						sent.reject(new TypeError(msg.reason));
						break;

					default:
						sent.reject(new Error(msg.reason));
				}
			}
			else
			{
				this._logger.error(
					'received response is not accepted nor rejected [method:%s, id:%s]',
					sent.method, sent.id);
			}
		}
		// If a notification emit it to the corresponding entity.
		else if (msg.targetId && msg.event)
		{
			this.emit(msg.targetId, msg.event, msg.data);
		}
		// Otherwise unexpected message.
		else
		{
			this._logger.error(
				'received message is not a response nor a notification');
		}
	}
}

module.exports = Channel;
