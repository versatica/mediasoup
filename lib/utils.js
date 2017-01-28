'use strict';

const randomString = require('random-string');
const randomNumber = require('random-number');
const clone = require('clone');

const randomNumberGenerator = randomNumber.generator(
	{
		min     : 10000000,
		max     : 99999999,
		integer : true
	});

module.exports =
{
	randomString(length)
	{
		return randomString({ numeric: false, length: length || 8 }).toLowerCase();
	},

	randomNumber: randomNumberGenerator,

	cloneObject(obj)
	{
		if (!obj || typeof obj !== 'object' || Array.isArray(obj))
			return {};
		else
			return clone(obj, false);
	}
};
