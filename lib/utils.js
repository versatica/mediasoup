const randomNumber = require('random-number');

const randomNumberGenerator = randomNumber.generator(
	{
		min     : 100000000,
		max     : 999999999,
		integer : true
	});

exports.randomNumber = randomNumberGenerator;

exports.cloneObject = function(obj)
{
	if (typeof obj !== 'object')
		return {};

	return JSON.parse(JSON.stringify(obj));
};
