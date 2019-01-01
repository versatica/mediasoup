const netstring = require('netstring');
const uuidv4 = require('uuid/v4');
const Logger = require('./Logger');
const EnhancedEventEmitter = require('./EnhancedEventEmitter');
const { InvalidStateError } = require('./errors');

// netstring length for a 65536 bytes payload.
const NS_MAX_SIZE = 65543;
// Max time waiting for a response from the worker subprocess.
const REQUEST_TIMEOUT = 20000;

const logger = new Logger('Channel');
const workerLogger = new Logger('worker');

class Channel extends EnhancedEventEmitter
{
	/**
	 * @private
	 */
	constructor({ socket })
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Unix Socket instance.
		// @type {net.Socket}
		this._socket = socket;

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

				if (this._recvBuffer.length > NS_MAX_SIZE)
				{
					logger.error('receiving buffer is full, discarding all data into it');

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
					logger.error('invalid data received: %s', String(error));

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
							workerLogger.debug(nsPayload.toString('utf8', 1));
							break;

						// 87 = 'W' (a warn log).
						case 87:
							workerLogger.warn(nsPayload.toString('utf8', 1));
							break;

						// 69 = 'E' (an error log).
						case 69:
							workerLogger.error(nsPayload.toString('utf8', 1));
							break;

						default:
							workerLogger.error(
								'unexpected data: %s', nsPayload.toString('utf8'));
					}
				}
				catch (error)
				{
					logger.error('received invalid message: %s', String(error));
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

		this._socket.on('end', () => logger.debug('pipe ended by the other side'));
		this._socket.on('error', (error) => logger.error('pipe error: %s', String(error)));
	}

	close()
	{
		if (this._closed)
			return;

		logger.debug('close()');

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

	async request(method, internal, data)
	{
		const id = uuidv4();

		logger.debug('request() [method:%s, id:%s]', method, id);

		if (this._closed)
			throw new InvalidStateError('closed');

		const request = { id, method, internal, data };
		const ns = netstring.nsWrite(JSON.stringify(request));

		if (Buffer.byteLength(ns) > NS_MAX_SIZE)
			throw new Error('request too big');

		// This may raise if closed or remote side ended.
		this._socket.write(ns);

		return new Promise((pResolve, pReject) =>
		{
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

					pReject(new Error('request timeout'));
				}, REQUEST_TIMEOUT),
				close : () =>
				{
					clearTimeout(sent.timer);
					pReject(new InvalidStateError('pipe closed'));
				}
			};

			// Add sent stuff to the Map.
			this._sents.set(id, sent);
		});
	}

	_processMessage(msg)
	{
		// If a response retrieve its associated request.
		if (msg.id)
		{
			if (msg.accepted)
				logger.debug('request succeeded [id:%s]', msg.id);
			else
				logger.error('request failed [id:%s, reason:"%s"]', msg.id, msg.reason);

			const sent = this._sents.get(msg.id);

			if (!sent)
			{
				logger.error('received response does not match any sent request');

				return;
			}

			if (msg.accepted)
			{
				sent.resolve(msg.data);
			}
			else if (msg.rejected)
			{
				logger.warn(
					'request failed [method:%s, id:%s]: %s',
					sent.method, sent.id, msg.reason);

				sent.reject(new Error(msg.reason));
			}
			else
			{
				logger.error('received response is not accepted nor rejected');
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
			logger.error('received message is not a response nor a notification');
		}
	}
}

module.exports = Channel;
