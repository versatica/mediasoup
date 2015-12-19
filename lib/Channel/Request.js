'use strict';

const randomNumber = require('random-number');
const logger = require('../logger')('Channel:Request');

const randomNumberGenerator = randomNumber.generator(
	{
		min     : 10000,
		max     : 99999,
		integer : true
	});

class Request
{
	constructor(method, data)
	{
		logger.debug('constructor() [method:%s]', method);

		this._id = randomNumberGenerator();
		this._method = method;
		this._data = data || {};
	}

	serialize()
	{
		return JSON.stringify(
			{
				id     : this._id,
				method : this._method,
				data   : this._data
			});
	}

	get id()
	{
		return this._id;
	}

	get method()
	{
		return this._method;
	}

	get data()
	{
		return this._data;
	}
}

module.exports = Request;
