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
 * Parse an array of StringUint8 into the corresponding object.
 */
export function parseMapStringUint8(
	binary: any, methodName: string
): Record<string, number>
{
	const map: Record<string, number> = {};

	for (let i=0; i<binary[`${methodName}Length`](); ++i)
	{
		const kv = binary[methodName](i)!;

		map[kv.key()] = kv.value()!;
	}

	return map;
}

/**
 * Parse an array of Uint32String into the corresponding object.
 */
export function parseMapUint32String(
	binary: any, methodName: string
): Record<number, string>
{
	const map: Record<number, string> = {};

	for (let i=0; i<binary[`${methodName}Length`](); ++i)
	{
		const kv = binary[methodName](i)!;

		map[kv.key()] = kv.value()!;
	}

	return map;
}
