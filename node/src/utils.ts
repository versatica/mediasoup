import { randomInt } from 'crypto';
import { arch, platform } from 'os';

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
export function generateRandomNumber()
{
	return randomInt(100_000_000, 999_999_999);
}

/**
 * Get the current platform triplet.
 *
 * @returns {string}
 */
export function getTriplet(): string
{
	return `${platform()}-${arch()}`;
}
