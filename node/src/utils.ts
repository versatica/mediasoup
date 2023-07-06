import { randomInt } from 'crypto';

/**
 * Generates a random positive integer.
 */
export function generateRandomNumber()
{
	return randomInt(100_000_000, 999_999_999);
}

type Only<T, U> =
{
	[P in keyof T]: T[P];
} &
{
	[P in keyof U]?: never;
};

export type Either<T, U> = Only<T, U> | Only<U, T>;
