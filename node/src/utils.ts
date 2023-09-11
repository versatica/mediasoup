import { randomInt } from 'crypto';
import { ProducerType } from './Producer';
import { Type as FbsRtpParametersType } from './fbs/rtp-parameters';

/**
 * Clones the given value.
 */
export function clone<T>(value: T): T
{
	if (value === undefined)
	{
		return undefined as unknown as T;
	}
	else if (Number.isNaN(value))
	{
		return NaN as unknown as T;
	}
	else if (typeof structuredClone === 'function')
	{
		// Available in Node >= 18.
		return structuredClone(value);
	}
	else
	{
		return JSON.parse(JSON.stringify(value));
	}
}

/**
 * Generates a random positive integer.
 */
export function generateRandomNumber(): number
{
	return randomInt(100_000_000, 999_999_999);
}

/**
 * Get the flatbuffers RtpParameters type for a given Producer.
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
 * Get the flatbuffers Producer type for a given RtpParameter type.
 */
export function getProducerType(
	rtpParameterType: FbsRtpParametersType
): ProducerType
{
	switch (rtpParameterType)
	{
		case FbsRtpParametersType.SIMPLE:
			return 'simple';

		case FbsRtpParametersType.SIMULCAST:
			return 'simulcast';

		case FbsRtpParametersType.SVC:
			return 'svc';

		default:
			throw new TypeError('invalid RtpParameter type');
	}
}

/**
 * Parse flatbuffers vector into an array of the given type.
 */
export function parseVector<Type>(
	binary: any, methodName: string, parseFn?: (binary2: any) => Type
): Type[]
{
	const array: Type[] = [];

	for (let i=0; i<binary[`${methodName}Length`](); ++i)
	{
		if (parseFn)
		{
			array.push(parseFn(binary[methodName](i)));
		}
		else
		{
			array.push(binary[methodName](i));
		}
	}

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
