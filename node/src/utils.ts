import * as randomNumber from 'random-number';

const randomNumberGenerator = randomNumber.generator(
	{
		min     : 100000000,
		max     : 999999999,
		integer : true
	});

/**
 * Clones the given object/array.
 */
export function clone(data: any): any
{
	if (typeof data !== 'object')
		return {};

	return JSON.parse(JSON.stringify(data));
}

/**
 * Generates a random positive integer.
 */
export { randomNumberGenerator as generateRandomNumber };
