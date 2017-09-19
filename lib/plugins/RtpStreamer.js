'use strict';

const Logger = require('../Logger');
const EnhancedEventEmitter = require('../EnhancedEventEmitter');

const logger = new Logger('RtpStreamer');

class RtpStreamer extends EnhancedEventEmitter
{
	constructor(consumer, transport)
	{
		super(logger);

		logger.debug('constructor()');

		// Closed flag.
		// @type {Boolean}
		this._closed = false;

		// Associated Consumer.
		// @type {Consumer}
		this._consumer = consumer;

		// Associated PlainRtpTransport.
		// @type {PlainRtpTransport}
		this._transport = transport;

		this._handleConsumer();
		this._handleTransport();
	}

	get consumer()
	{
		return this._consumer;
	}

	get transport()
	{
		return this._transport;
	}

	close()
	{
		logger.debug('close()');

		if (this._closed)
			return;

		this._closed = true;

		if (!this._consumer.closed)
			this._consumer.close();

		if (!this._transport.closed)
			this._transport.close();

		this.emit('close');
	}

	_handleConsumer()
	{
		this._consumer.on('@close', () =>
		{
			if (!this._closed)
				this.close();
		});
	}

	_handleTransport()
	{
		this._transport.on('@close', () =>
		{
			if (!this._closed)
				this.close();
		});
	}
}

module.exports = RtpStreamer;
