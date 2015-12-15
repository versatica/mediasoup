'use strict';

const EventEmitter = require('events').EventEmitter;
const netstring = require('netstring');
const logger = require('../logger')('Channel');
const Request = require('./Request');

class Channel extends EventEmitter
{
	constructor(socket)
	{
		super();

		logger.debug('constructor()');

		// Unix Socket instance
		this._socket = socket;

		// Map of sent requests waiting for a response
		this._pendingRequests = new Map();
	}

	sendRequest(method, data)
	{
		logger.debug('sendRequest() [method:%s]', method);

		let request = new Request(method, data);
		let json = JSON.stringify(request);
		let ns = netstring.nsWrite(json);

		logger.debug('nsWriteLength: %s', netstring.nsWriteLength(Buffer.byteLength(json)));

		// TODO: This may raise if closed or remote side ended
		this._socket.write(ns);
	}
}

module.exports = Channel;
