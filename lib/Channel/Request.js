'use strict';

const randomString = require('random-string');
const logger = require('../logger')('Channel:Request');

class Request
{
	constructor(method, data)
	{
		logger.debug('constructor() [method:%s]', method);

		this._id = randomString({ length: 8 }).toLowerCase();
		this._method = method;
		this._data = data || {};
	}

	serialize()
	{
		return JSON.stringify(
			{
				method : this._method,
				id     : this._id,
				data   : this._data
			});
	}

	get method()
	{
		return this._method;
	}

	get id()
	{
		return this._id;
	}

	get data()
	{
		return this._data;
	}
}

module.exports = Request;
