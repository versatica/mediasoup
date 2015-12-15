'use strict';

const randomString = require('random-string');
const logger = require('../logger')('Channel:Request');

class Request
{
	constructor(method, data)
	{
		logger.debug('constructor() [method:%s]', method);

		this.id = randomString({ length: 8 }).toLowerCase();
		this.method = method;
		this.data = data || {};
	}
}

module.exports = Request;
