import { ProducerType } from '../Producer';
import { Type as FbsRtpParametersType } from './fbs/rtp-parameters/type';

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
 * Parse vector into an array of the given type.
 */
export function parseVector<Type>(binary: any, methodName: string): Type[]
{
	const array: Type[] = [];

	for (let i=0; i<binary[`${methodName}Length`](); ++i)
		array.push(binary[methodName](i));

	return array;
}

/**
 * Parse an vector of StringUint8 into the corresponding array.
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
 * Parse a vector of Uint32String into the corresponding array.
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
