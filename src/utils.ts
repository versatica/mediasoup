import * as randomNumber from 'random-number';
import { networkInterfaces } from 'os';

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

/**
 * Get IPv4 IP addresses from all network interfaces available
 */
export function getIpv4IPs(): string[]
{
	const ips = [];

	for (const networkInterface of Object.values(networkInterfaces()))
	{
		for (const info of networkInterface ?? [])
		{
			if (info.family === 'IPv4')
			{
				ips.push(info.address);
			}
		}
	}

	return ips;
}

/**
 * Get IPv4 and IPv6 IP addresses from all network interfaces available
 */
export function getAllIPs(): string[]
{
	const ips = [];

	for (const networkInterface of Object.values(networkInterfaces()))
	{
		for (const info of networkInterface ?? [])
		{
			ips.push(info.address);
		}
	}

	return ips;
}

export function flatten<T>(arr: Array<T | T[]>): T[]
{
	// @ts-ignore Typing this correctly seems impossible
	return [].concat(...arr);
}
