import type {
	RtpStreamDump,
	RtpStreamParametersDump,
	RtxStreamDump,
	RtxStreamParameters,
} from './ConsumerTypes';
import * as FbsRtpStream from './fbs/rtp-stream';
import * as FbsRtxStream from './fbs/rtx-stream';

export function parseRtpStream(data: FbsRtpStream.Dump): RtpStreamDump {
	const params = parseRtpStreamParameters(data.params()!);

	let rtxStream: RtxStreamDump | undefined;

	if (data.rtxStream()) {
		rtxStream = parseRtxStream(data.rtxStream()!);
	}

	return {
		params,
		score: data.score(),
		rtxStream,
	};
}

function parseRtpStreamParameters(
	data: FbsRtpStream.Params
): RtpStreamParametersDump {
	return {
		encodingIdx: data.encodingIdx(),
		ssrc: data.ssrc(),
		payloadType: data.payloadType(),
		mimeType: data.mimeType()!,
		clockRate: data.clockRate(),
		rid: data.rid()!.length > 0 ? data.rid()! : undefined,
		cname: data.cname()!,
		rtxSsrc: data.rtxSsrc() !== null ? data.rtxSsrc()! : undefined,
		rtxPayloadType:
			data.rtxPayloadType() !== null ? data.rtxPayloadType()! : undefined,
		useNack: data.useNack(),
		usePli: data.usePli(),
		useFir: data.useFir(),
		useInBandFec: data.useInBandFec(),
		useDtx: data.useDtx(),
		spatialLayers: data.spatialLayers(),
		temporalLayers: data.temporalLayers(),
	};
}

function parseRtxStream(data: FbsRtxStream.RtxDump): RtxStreamDump {
	const params = parseRtxStreamParameters(data.params()!);

	return {
		params,
	};
}

function parseRtxStreamParameters(
	data: FbsRtxStream.Params
): RtxStreamParameters {
	return {
		ssrc: data.ssrc(),
		payloadType: data.payloadType(),
		mimeType: data.mimeType()!,
		clockRate: data.clockRate(),
		rrid: data.rrid()!.length > 0 ? data.rrid()! : undefined,
		cname: data.cname()!,
	};
}
