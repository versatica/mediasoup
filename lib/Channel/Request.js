'use strict';

const logger = require('../logger')('Channel:Request');
const utils = require('../utils');

class Request
{
	constructor(method, data)
	{
		logger.debug('constructor() [method:%s]', method);

		this._id = utils.randomNumber();
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
