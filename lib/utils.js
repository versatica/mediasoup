'use strict';

const randomString = require('random-string');
const randomNumber = require('random-number');
const clone = require('clone');

const randomNumberGenerator = randomNumber.generator(
	{
		min     : 10000,
		max     : 99999,
		integer : true
	});

module.exports =
{
	randomString: () =>
	{
		return randomString({ numeric: false, length: 6 }).toLowerCase();
	},

	randomNumber: randomNumberGenerator,

	clone: (obj) =>
	{
		return clone(obj, false);
	}
};
