import type * as flatbuffers from 'flatbuffers';
import type { RtpCodecsEncodingsMapping } from './ortc';
import * as fbsUtils from './fbsUtils';
import * as FbsRtpParameters from './fbs/rtp-parameters';

export function serializeRtpMapping(
	builder: flatbuffers.Builder,
	rtpMapping: RtpCodecsEncodingsMapping
): number {
	const codecs: number[] = [];

	for (const codec of rtpMapping.codecs) {
		codecs.push(
			FbsRtpParameters.CodecMapping.createCodecMapping(
				builder,
				codec.payloadType,
				codec.mappedPayloadType
			)
		);
	}
	const codecsOffset = FbsRtpParameters.RtpMapping.createCodecsVector(
		builder,
		codecs
	);

	const encodings: number[] = [];

	for (const encoding of rtpMapping.encodings) {
		encodings.push(
			FbsRtpParameters.EncodingMapping.createEncodingMapping(
				builder,
				builder.createString(encoding.rid),
				encoding.ssrc ?? null,
				encoding.mappedSsrc
			)
		);
	}

	const encodingsOffset = FbsRtpParameters.RtpMapping.createEncodingsVector(
		builder,
		encodings
	);

	return FbsRtpParameters.RtpMapping.createRtpMapping(
		builder,
		codecsOffset,
		encodingsOffset
	);
}

export function parseRtpMapping(
	data: FbsRtpParameters.RtpMapping
): RtpCodecsEncodingsMapping {
	return {
		codecs: fbsUtils.parseVector(
			data,
			'codecs',
			(codec: FbsRtpParameters.CodecMapping) => ({
				payloadType: codec.payloadType(),
				mappedPayloadType: codec.mappedPayloadType(),
			})
		),
		encodings: fbsUtils.parseVector(
			data,
			'encodings',
			(encoding: FbsRtpParameters.EncodingMapping) => ({
				ssrc: encoding.ssrc() ?? undefined,
				rid: encoding.rid() || undefined,
				mappedSsrc: encoding.mappedSsrc(),
			})
		),
	};
}
