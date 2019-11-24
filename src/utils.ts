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
export function clone(obj: any[] | object): any[] | object
{
	if (typeof obj !== 'object')
		return {};

	return JSON.parse(JSON.stringify(obj));
}

/**
 * Generates a random positive integer.
 */
export { randomNumberGenerator as generateRandomNumber };
