'use strict';

const randomatic = require('randomatic');
const randomNumber = require('random-number');
const clone = require('clone');

const randomNumberGenerator = randomNumber.generator(
	{
		min     : 10000000,
		max     : 99999999,
		integer : true
	});

exports.randomString = function(length)
{
	return randomatic('a', length || 8);
};

exports.randomNumber = randomNumberGenerator;

exports.cloneObject = function(obj)
{
	if (typeof obj !== 'object')
		return {};

	return clone(obj, false);
};
