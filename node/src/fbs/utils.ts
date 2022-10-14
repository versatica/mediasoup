import * as flatbuffers from 'flatbuffers';
import { RtpEncodingParameters, RtpParameters } from '../RtpParameters';
import { ProducerType } from '../Producer';
import { RtpParameters as FbsRtpParameters } from './f-b-s/rtp-parameters/rtp-parameters';
import { Type as FbsRtpParametersType } from './f-b-s/rtp-parameters/type';
import {
	Double as FbsDouble,
	Integer as FbsInteger,
	IntegerArray as FbsIntegerArray,
	Parameter as FbsParameter,
	RtcpFeedback as FbsRtcpFeedback,
	RtcpParameters as FbsRtcpParameters,
	RtpCodecParameters as FbsRtpCodecParameters,
	RtpEncodingParameters as FbsRtpEncodingParameters,
	RtpHeaderExtensionParameters as FbsRtpHeaderExtensionParameters,
	Rtx as FbsRtx,
	Value as FbsValue
} from './rtpParameters';

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
 * Get array of type T from a flatbuffer arrays of T.
 */
export function getArray<T>(holder: any, field: string): T[]
{
	const arr: T[] = [];

	for (let idx=0; idx < holder[`${field}Length`](); ++idx)
	{
		arr.push(holder[field](idx));
	}

	return arr;
}

export function serializeRtpParameters(
	builder: flatbuffers.Builder, rtpParameters: RtpParameters
): number
{
	const codecs: number[] = [];
	const headerExtensions: number[] = [];

	console.error("serializeRtpParameters 1");
	for (const codec of rtpParameters.codecs)
	{
		const mimeTypeOffset = builder.createString(codec.mimeType);

		const codecParameters: number[] = [];

		for (const key of Object.keys(codec.parameters))
		{
			const value = codec.parameters[key];
			const keyOffset = builder.createString(key);
			let parameterOffset: number;

			if (typeof value === 'boolean')
			{
				parameterOffset = FbsParameter.createParameter(
					builder, keyOffset, FbsValue.Boolean, value === true ? 1:0
				);
			}

			else if (typeof value === 'number')
			{
				// Integer.
				if (value % 1 === 0)
				{
					const valueOffset = FbsInteger.createInteger(builder, value);

					parameterOffset = FbsParameter.createParameter(
						builder, keyOffset, FbsValue.Integer, valueOffset
					);
				}
				// Float.
				else
				{
					const valueOffset = FbsDouble.createDouble(builder, value);

					parameterOffset = FbsParameter.createParameter(
						builder, keyOffset, FbsValue.Double, valueOffset
					);
				}
			}

			else if (typeof value === 'string')
			{
				const valueOffset = builder.createString(value);

				parameterOffset = FbsParameter.createParameter(
					builder, keyOffset, FbsValue.String, valueOffset
				);
			}

			else if (Array.isArray(value))
			{
				const valueOffset = FbsIntegerArray.createValueVector(builder, value);

				parameterOffset = FbsParameter.createParameter(
					builder, keyOffset, FbsValue.IntegerArray, valueOffset
				);
			}

			else
			{
				throw new Error(`invalid parameter type [key:'${key}', value:${value}]`);
			}

			codecParameters.push(parameterOffset);
		}
		const parametersOffset =
			FbsRtpCodecParameters.createParametersVector(builder, codecParameters);

		const rtcpFeedback: number[] = [];

		for (const rtcp of codec.rtcpFeedback?? [])
		{
			const typeOffset = builder.createString(rtcp.type);
			const rtcpParametersOffset = builder.createString(rtcp.parameter);

			rtcpFeedback.push(
				FbsRtcpFeedback.createRtcpFeedback(builder, typeOffset, rtcpParametersOffset));
		}
		const rtcpFeedbackOffset =
			FbsRtpCodecParameters.createRtcpFeedbackVector(builder, rtcpFeedback);

		codecs.push(
			FbsRtpCodecParameters.createRtpCodecParameters(
				builder,
				mimeTypeOffset,
				codec.payloadType,
				codec.clockRate,
				Number(codec.channels),
				parametersOffset,
				rtcpFeedbackOffset
			));
	}
	const codecsOffset = FbsRtpParameters.createCodecsVector(builder, codecs);

	console.error("serializeRtpParameters 2");
	// RtpHeaderExtensionParameters.
	for (const headerExtension of rtpParameters.headerExtensions ?? [])
	{
		const uriOffset = builder.createString(headerExtension.uri);
		const parametersOffset = builder.createString(headerExtension.parameters);

		headerExtensions.push(
			FbsRtpHeaderExtensionParameters.createRtpHeaderExtensionParameters(
				builder,
				uriOffset,
				headerExtension.id,
				Boolean(headerExtension.encrypt),
				parametersOffset));
	}
	const headerExtensionsOffset =
		FbsRtpParameters.createHeaderExtensionsVector(builder, headerExtensions);

	console.error("serializeRtpParameters 3");
	// RtpEncodingParameters.
	let encodingsOffset: number | undefined;

	if (rtpParameters.encodings)
		encodingsOffset = serializeRtpEncodingParameters(builder, rtpParameters.encodings);

	console.error("serializeRtpParameters 3.1");
	// RtcpParameters.
	let rtcpOffset: number | undefined;

	if (rtpParameters.rtcp)
	{
		const { cname, reducedSize, mux } = rtpParameters.rtcp;
		const cnameOffset = builder.createString(cname);

		rtcpOffset = FbsRtcpParameters.createRtcpParameters(
			builder, cnameOffset, Boolean(reducedSize), Boolean(mux)
		);
	}

	console.error("serializeRtpParameters 4");
	const midOffset = builder.createString(rtpParameters.mid);

	FbsRtpParameters.startRtpParameters(builder);
	FbsRtpParameters.addMid(builder, midOffset);
	FbsRtpParameters.addCodecs(builder, codecsOffset);

	if (headerExtensions.length > 0)
		FbsRtpParameters.addHeaderExtensions(builder, headerExtensionsOffset);

	if (encodingsOffset)
		FbsRtpParameters.addEncodings(builder, encodingsOffset);

	if (rtcpOffset)
		FbsRtpParameters.addRtcp(builder, rtcpOffset);

	console.error("serializeRtpParameters 5");
	return FbsRtpParameters.endRtpParameters(builder);
}

export function serializeRtpEncodingParameters(
	builder: flatbuffers.Builder, rtpEncodingParameters: RtpEncodingParameters[]
): number
{
	const encodings: number[] = [];

	for (const encoding of rtpEncodingParameters ?? [])
	{
		// Prepare Rid.
		const ridOffset = builder.createString(encoding.rid);

		// Prepare Rtx.
		let rtxOffset: number | undefined;

		if (encoding.rtx)
			FbsRtx.createRtx(builder, encoding.rtx.ssrc);

		// Prepare scalability mode.
		let scalabilityModeOffset: number | undefined;

		if (encoding.scalabilityMode)
			scalabilityModeOffset = builder.createString(encoding.scalabilityMode);

		// Start serialization.
		FbsRtpEncodingParameters.startRtpEncodingParameters(builder);

		// Add SSRC.
		if (encoding.ssrc)
			FbsRtpEncodingParameters.addSsrc(builder, encoding.ssrc);

		// Add Rid.
		FbsRtpEncodingParameters.addRid(builder, ridOffset);

		// Add payload type.
		if (encoding.codecPayloadType)
			FbsRtpEncodingParameters.addCodecPayloadType(builder, encoding.codecPayloadType);

		// Add RTX.
		if (rtxOffset)
			FbsRtpEncodingParameters.addRtx(builder, rtxOffset);

		// Add DTX.
		if (encoding.dtx !== undefined)
			FbsRtpEncodingParameters.addDtx(builder, encoding.dtx);

		// Add scalability ode.
		if (scalabilityModeOffset)
			FbsRtpEncodingParameters.addScalabilityMode(builder, scalabilityModeOffset);

		// Add scale resolution down by.
		if (encoding.scaleResolutionDownBy !== undefined)
		{
			FbsRtpEncodingParameters.addScaleResolutionDownBy(
				builder, encoding.scaleResolutionDownBy);
		}

		// Add max bitrate.
		if (encoding.maxBitrate !== undefined)
			FbsRtpEncodingParameters.addMaxBitrate(builder, encoding.maxBitrate);

		// End serialization.
		encodings.push(FbsRtpEncodingParameters.endRtpEncodingParameters(builder));
	}

	return FbsRtpParameters.createEncodingsVector(builder, encodings);
}
