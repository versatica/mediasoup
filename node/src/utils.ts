import { randomInt } from 'crypto';
import { ProducerType } from './Producer';
import { Type as FbsRtpParametersType } from './fbs/fbs/rtp-parameters/type';

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

type Only<T, U> = {
	[P in keyof T]: T[P];
} & {
	[P in keyof U]?: never;
};

export type Either<T, U> = Only<T, U> | Only<U, T>;

/**
 * Get the flatbuffers RtpParameters type of a given Producer.
 */
export function getRtpParametersType(
	producerType: ProducerType, pipe: boolean
): FbsRtpParametersType
{
	if (pipe)
	{
		return FbsRtpParametersType.PIPE;
	}

	switch (producerType)
	{
		case 'simple':
			return FbsRtpParametersType.SIMPLE;

		case 'simulcast':
			return FbsRtpParametersType.SIMULCAST;

		case 'svc':
			return FbsRtpParametersType.SVC;

		default:
			return FbsRtpParametersType.NONE;
	}
}

/**
 * Parse flatbuffers vector into an array of the given type.
 */
export function parseVector<Type>(binary: any, methodName: string): Type[]
{
	const array: Type[] = [];

	for (let i=0; i<binary[`${methodName}Length`](); ++i)
		array.push(binary[methodName](i));

	return array;
}

/**
 * Parse flatbuffers vector of StringUint8 into the corresponding array.
 */
export function parseStringUint8Vector(
	binary: any, methodName: string
): {key: string; value: number}[]
{
	const array: {key: string; value: number}[] = [];

	for (let i=0; i<binary[`${methodName}Length`](); ++i)
	{
		const kv = binary[methodName](i)!;

		array.push({ key: kv.key(), value: kv.value() });
	}

	return array;
}

/**
 * Parse flatbuffers of Uint16String into the corresponding array.
 */
export function parseUint16StringVector(
	binary: any, methodName: string
): { key: number; value: string }[]
{
	const array: { key: number; value: string }[] = [];

	for (let i=0; i<binary[`${methodName}Length`](); ++i)
	{
		const kv = binary[methodName](i)!;

		array.push({ key: kv.key(), value: kv.value() });
	}

	return array;
}

/**
 * Parse flatbuffers of Uint32String into the corresponding array.
 */
export function parseUint32StringVector(
	binary: any, methodName: string
): { key: number; value: string }[]
{
	const array: { key: number; value: string }[] = [];

	for (let i=0; i<binary[`${methodName}Length`](); ++i)
	{
		const kv = binary[methodName](i)!;

		array.push({ key: kv.key(), value: kv.value() });
	}

	return array;
}
