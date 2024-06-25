import * as flatbuffers from 'flatbuffers';
import {
	Boolean as FbsBoolean,
	Double as FbsDouble,
	Integer32 as FbsInteger32,
	Integer32Array as FbsInteger32Array,
	String as FbsString,
	Parameter as FbsParameter,
	RtcpFeedback as FbsRtcpFeedback,
	RtcpParameters as FbsRtcpParameters,
	RtpCodecParameters as FbsRtpCodecParameters,
	RtpEncodingParameters as FbsRtpEncodingParameters,
	RtpHeaderExtensionParameters as FbsRtpHeaderExtensionParameters,
	RtpHeaderExtensionUri as FbsRtpHeaderExtensionUri,
	RtpParameters as FbsRtpParameters,
	Rtx as FbsRtx,
	Value as FbsValue,
} from './fbs/rtp-parameters';
import * as utils from './utils';

/**
 * The RTP capabilities define what mediasoup or an endpoint can receive at
 * media level.
 */
export type RtpCapabilities = {
	/**
	 * Supported media and RTX codecs.
	 */
	codecs?: RtpCodecCapability[];

	/**
	 * Supported RTP header extensions.
	 */
	headerExtensions?: RtpHeaderExtension[];
};

/**
 * Media kind ('audio' or 'video').
 */
export type MediaKind = 'audio' | 'video';

/**
 * Provides information on the capabilities of a codec within the RTP
 * capabilities. The list of media codecs supported by mediasoup and their
 * settings is defined in the supportedRtpCapabilities.ts file.
 *
 * Exactly one RtpCodecCapability will be present for each supported combination
 * of parameters that requires a distinct value of preferredPayloadType. For
 * example:
 *
 * - Multiple H264 codecs, each with their own distinct 'packetization-mode' and
 *   'profile-level-id' values.
 * - Multiple VP9 codecs, each with their own distinct 'profile-id' value.
 *
 * RtpCodecCapability entries in the mediaCodecs array of RouterOptions do not
 * require preferredPayloadType field (if unset, mediasoup will choose a random
 * one). If given, make sure it's in the 96-127 range.
 */
export type RtpCodecCapability = {
	/**
	 * Media kind.
	 */
	kind: MediaKind;

	/**
	 * The codec MIME media type/subtype (e.g. 'audio/opus', 'video/VP8').
	 */
	mimeType: string;

	/**
	 * The preferred RTP payload type.
	 */
	preferredPayloadType?: number;

	/**
	 * Codec clock rate expressed in Hertz.
	 */
	clockRate: number;

	/**
	 * The number of channels supported (e.g. two for stereo). Just for audio.
	 * Default 1.
	 */
	channels?: number;

	/**
	 * Codec specific parameters. Some parameters (such as 'packetization-mode'
	 * and 'profile-level-id' in H264 or 'profile-id' in VP9) are critical for
	 * codec matching.
	 */
	parameters?: any;

	/**
	 * Transport layer and codec-specific feedback messages for this codec.
	 */
	rtcpFeedback?: RtcpFeedback[];
};

/**
 * Direction of RTP header extension.
 */
export type RtpHeaderExtensionDirection =
	| 'sendrecv'
	| 'sendonly'
	| 'recvonly'
	| 'inactive';

/**
 * Provides information relating to supported header extensions. The list of
 * RTP header extensions supported by mediasoup is defined in the
 * supportedRtpCapabilities.ts file.
 *
 * mediasoup does not currently support encrypted RTP header extensions. The
 * direction field is just present in mediasoup RTP capabilities (retrieved via
 * router.rtpCapabilities or mediasoup.getSupportedRtpCapabilities()). It's
 * ignored if present in endpoints' RTP capabilities.
 */
export type RtpHeaderExtension = {
	/**
	 * Media kind.
	 */
	kind: MediaKind;

	/*
	 * The URI of the RTP header extension, as defined in RFC 5285.
	 */
	uri: RtpHeaderExtensionUri;

	/**
	 * The preferred numeric identifier that goes in the RTP packet. Must be
	 * unique.
	 */
	preferredId: number;

	/**
	 * If true, it is preferred that the value in the header be encrypted as per
	 * RFC 6904. Default false.
	 */
	preferredEncrypt?: boolean;

	/**
	 * If 'sendrecv', mediasoup supports sending and receiving this RTP extension.
	 * 'sendonly' means that mediasoup can send (but not receive) it. 'recvonly'
	 * means that mediasoup can receive (but not send) it.
	 */
	direction?: RtpHeaderExtensionDirection;
};

/**
 * The RTP send parameters describe a media stream received by mediasoup from
 * an endpoint through its corresponding mediasoup Producer. These parameters
 * may include a mid value that the mediasoup transport will use to match
 * received RTP packets based on their MID RTP extension value.
 *
 * mediasoup allows RTP send parameters with a single encoding and with multiple
 * encodings (simulcast). In the latter case, each entry in the encodings array
 * must include a ssrc field or a rid field (the RID RTP extension value). Check
 * the Simulcast and SVC sections for more information.
 *
 * The RTP receive parameters describe a media stream as sent by mediasoup to
 * an endpoint through its corresponding mediasoup Consumer. The mid value is
 * unset (mediasoup does not include the MID RTP extension into RTP packets
 * being sent to endpoints).
 *
 * There is a single entry in the encodings array (even if the corresponding
 * producer uses simulcast). The consumer sends a single and continuous RTP
 * stream to the endpoint and spatial/temporal layer selection is possible via
 * consumer.setPreferredLayers().
 *
 * As an exception, previous bullet is not true when consuming a stream over a
 * PipeTransport, in which all RTP streams from the associated producer are
 * forwarded verbatim through the consumer.
 *
 * The RTP receive parameters will always have their ssrc values randomly
 * generated for all of its  encodings (and optional rtx: { ssrc: XXXX } if the
 * endpoint supports RTX), regardless of the original RTP send parameters in
 * the associated producer. This applies even if the producer's encodings have
 * rid set.
 */
export type RtpParameters = {
	/**
	 * The MID RTP extension value as defined in the BUNDLE specification.
	 */
	mid?: string;

	/**
	 * Media and RTX codecs in use.
	 */
	codecs: RtpCodecParameters[];

	/**
	 * RTP header extensions in use.
	 */
	headerExtensions?: RtpHeaderExtensionParameters[];

	/**
	 * Transmitted RTP streams and their settings.
	 */
	encodings?: RtpEncodingParameters[];

	/**
	 * Parameters used for RTCP.
	 */
	rtcp?: RtcpParameters;
};

/**
 * Provides information on codec settings within the RTP parameters. The list
 * of media codecs supported by mediasoup and their settings is defined in the
 * supportedRtpCapabilities.ts file.
 */
export type RtpCodecParameters = {
	/**
	 * The codec MIME media type/subtype (e.g. 'audio/opus', 'video/VP8').
	 */
	mimeType: string;

	/**
	 * The value that goes in the RTP Payload Type Field. Must be unique.
	 */
	payloadType: number;

	/**
	 * Codec clock rate expressed in Hertz.
	 */
	clockRate: number;

	/**
	 * The number of channels supported (e.g. two for stereo). Just for audio.
	 * Default 1.
	 */
	channels?: number;

	/**
	 * Codec-specific parameters available for signaling. Some parameters (such
	 * as 'packetization-mode' and 'profile-level-id' in H264 or 'profile-id' in
	 * VP9) are critical for codec matching.
	 */
	parameters?: any;

	/**
	 * Transport layer and codec-specific feedback messages for this codec.
	 */
	rtcpFeedback?: RtcpFeedback[];
};

/**
 * Provides information on RTCP feedback messages for a specific codec. Those
 * messages can be transport layer feedback messages or codec-specific feedback
 * messages. The list of RTCP feedbacks supported by mediasoup is defined in the
 * supportedRtpCapabilities.ts file.
 */
export type RtcpFeedback = {
	/**
	 * RTCP feedback type.
	 */
	type: string;

	/**
	 * RTCP feedback parameter.
	 */
	parameter?: string;
};

/**
 * Provides information relating to an encoding, which represents a media RTP
 * stream and its associated RTX stream (if any).
 */
export type RtpEncodingParameters = {
	/**
	 * The media SSRC.
	 */
	ssrc?: number;

	/**
	 * The RID RTP extension value. Must be unique.
	 */
	rid?: string;

	/**
	 * Codec payload type this encoding affects. If unset, first media codec is
	 * chosen.
	 */
	codecPayloadType?: number;

	/**
	 * RTX stream information. It must contain a numeric ssrc field indicating
	 * the RTX SSRC.
	 */
	rtx?: { ssrc: number };

	/**
	 * It indicates whether discontinuous RTP transmission will be used. Useful
	 * for audio (if the codec supports it) and for video screen sharing (when
	 * static content is being transmitted, this option disables the RTP
	 * inactivity checks in mediasoup). Default false.
	 */
	dtx?: boolean;

	/**
	 * Number of spatial and temporal layers in the RTP stream (e.g. 'L1T3').
	 * See webrtc-svc.
	 */
	scalabilityMode?: string;

	/**
	 * Others.
	 */
	scaleResolutionDownBy?: number;
	maxBitrate?: number;
};

export type RtpHeaderExtensionUri =
	| 'urn:ietf:params:rtp-hdrext:sdes:mid'
	| 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id'
	| 'urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id'
	| 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07'
	| 'urn:ietf:params:rtp-hdrext:framemarking'
	| 'urn:ietf:params:rtp-hdrext:ssrc-audio-level'
	| 'urn:3gpp:video-orientation'
	| 'urn:ietf:params:rtp-hdrext:toffset'
	| 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01'
	| 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time'
	| 'http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time'
	| 'http://www.webrtc.org/experiments/rtp-hdrext/playout-delay';

/**
 * Defines a RTP header extension within the RTP parameters. The list of RTP
 * header extensions supported by mediasoup is defined in the
 * supportedRtpCapabilities.ts file.
 *
 * mediasoup does not currently support encrypted RTP header extensions and no
 * parameters are currently considered.
 */
export type RtpHeaderExtensionParameters = {
	/**
	 * The URI of the RTP header extension, as defined in RFC 5285.
	 */
	uri: RtpHeaderExtensionUri;

	/**
	 * The numeric identifier that goes in the RTP packet. Must be unique.
	 */
	id: number;

	/**
	 * If true, the value in the header is encrypted as per RFC 6904. Default false.
	 */
	encrypt?: boolean;

	/**
	 * Configuration parameters for the header extension.
	 */
	parameters?: any;
};

/**
 * Provides information on RTCP settings within the RTP parameters.
 *
 * If no cname is given in a producer's RTP parameters, the mediasoup transport
 * will choose a random one that will be used into RTCP SDES messages sent to
 * all its associated consumers.
 *
 * mediasoup assumes reducedSize to always be true.
 */
export type RtcpParameters = {
	/**
	 * The Canonical Name (CNAME) used by RTCP (e.g. in SDES messages).
	 */
	cname?: string;

	/**
	 * Whether reduced size RTCP RFC 5506 is configured (if true) or compound RTCP
	 * as specified in RFC 3550 (if false). Default true.
	 */
	reducedSize?: boolean;
};

export function serializeRtpParameters(
	builder: flatbuffers.Builder,
	rtpParameters: RtpParameters
): number {
	const codecs: number[] = [];
	const headerExtensions: number[] = [];

	for (const codec of rtpParameters.codecs) {
		const mimeTypeOffset = builder.createString(codec.mimeType);
		const parameters = serializeParameters(builder, codec.parameters);
		const parametersOffset = FbsRtpCodecParameters.createParametersVector(
			builder,
			parameters
		);

		const rtcpFeedback: number[] = [];

		for (const rtcp of codec.rtcpFeedback ?? []) {
			const typeOffset = builder.createString(rtcp.type);
			const rtcpParametersOffset = builder.createString(rtcp.parameter);

			rtcpFeedback.push(
				FbsRtcpFeedback.createRtcpFeedback(
					builder,
					typeOffset,
					rtcpParametersOffset
				)
			);
		}
		const rtcpFeedbackOffset = FbsRtpCodecParameters.createRtcpFeedbackVector(
			builder,
			rtcpFeedback
		);

		codecs.push(
			FbsRtpCodecParameters.createRtpCodecParameters(
				builder,
				mimeTypeOffset,
				codec.payloadType,
				codec.clockRate,
				Number(codec.channels),
				parametersOffset,
				rtcpFeedbackOffset
			)
		);
	}
	const codecsOffset = FbsRtpParameters.createCodecsVector(builder, codecs);

	// RtpHeaderExtensionParameters.
	for (const headerExtension of rtpParameters.headerExtensions ?? []) {
		const uri = rtpHeaderExtensionUriToFbs(headerExtension.uri);
		const parameters = serializeParameters(builder, headerExtension.parameters);
		const parametersOffset = FbsRtpCodecParameters.createParametersVector(
			builder,
			parameters
		);

		headerExtensions.push(
			FbsRtpHeaderExtensionParameters.createRtpHeaderExtensionParameters(
				builder,
				uri,
				headerExtension.id,
				Boolean(headerExtension.encrypt),
				parametersOffset
			)
		);
	}
	const headerExtensionsOffset = FbsRtpParameters.createHeaderExtensionsVector(
		builder,
		headerExtensions
	);

	// RtpEncodingParameters.
	const encodingsOffset = serializeRtpEncodingParameters(
		builder,
		rtpParameters.encodings ?? []
	);

	// RtcpParameters.
	const { cname, reducedSize } = rtpParameters.rtcp ?? { reducedSize: true };
	const cnameOffset = builder.createString(cname);

	const rtcpOffset = FbsRtcpParameters.createRtcpParameters(
		builder,
		cnameOffset,
		Boolean(reducedSize)
	);

	const midOffset = builder.createString(rtpParameters.mid);

	FbsRtpParameters.startRtpParameters(builder);
	FbsRtpParameters.addMid(builder, midOffset);
	FbsRtpParameters.addCodecs(builder, codecsOffset);

	FbsRtpParameters.addHeaderExtensions(builder, headerExtensionsOffset);
	FbsRtpParameters.addEncodings(builder, encodingsOffset);
	FbsRtpParameters.addRtcp(builder, rtcpOffset);

	return FbsRtpParameters.endRtpParameters(builder);
}

export function serializeRtpEncodingParameters(
	builder: flatbuffers.Builder,
	rtpEncodingParameters: RtpEncodingParameters[] = []
): number {
	const encodings: number[] = [];

	for (const encoding of rtpEncodingParameters) {
		// Prepare Rid.
		const ridOffset = builder.createString(encoding.rid);

		// Prepare Rtx.
		let rtxOffset: number | undefined;

		if (encoding.rtx) {
			rtxOffset = FbsRtx.createRtx(builder, encoding.rtx.ssrc);
		}

		// Prepare scalability mode.
		let scalabilityModeOffset: number | undefined;

		if (encoding.scalabilityMode) {
			scalabilityModeOffset = builder.createString(encoding.scalabilityMode);
		}

		// Start serialization.
		FbsRtpEncodingParameters.startRtpEncodingParameters(builder);

		// Add SSRC.
		if (encoding.ssrc) {
			FbsRtpEncodingParameters.addSsrc(builder, encoding.ssrc);
		}

		// Add Rid.
		FbsRtpEncodingParameters.addRid(builder, ridOffset);

		// Add payload type.
		if (encoding.codecPayloadType) {
			FbsRtpEncodingParameters.addCodecPayloadType(
				builder,
				encoding.codecPayloadType
			);
		}

		// Add RTX.
		if (rtxOffset) {
			FbsRtpEncodingParameters.addRtx(builder, rtxOffset);
		}

		// Add DTX.
		if (encoding.dtx !== undefined) {
			FbsRtpEncodingParameters.addDtx(builder, encoding.dtx);
		}

		// Add scalability ode.
		if (scalabilityModeOffset) {
			FbsRtpEncodingParameters.addScalabilityMode(
				builder,
				scalabilityModeOffset
			);
		}

		// Add max bitrate.
		if (encoding.maxBitrate !== undefined) {
			FbsRtpEncodingParameters.addMaxBitrate(builder, encoding.maxBitrate);
		}

		// End serialization.
		encodings.push(FbsRtpEncodingParameters.endRtpEncodingParameters(builder));
	}

	return FbsRtpParameters.createEncodingsVector(builder, encodings);
}

export function serializeParameters(
	builder: flatbuffers.Builder,
	parameters: any
): number[] {
	const fbsParameters: number[] = [];

	for (const key of Object.keys(parameters)) {
		const value = parameters[key];
		const keyOffset = builder.createString(key);
		let parameterOffset: number;

		if (typeof value === 'boolean') {
			parameterOffset = FbsParameter.createParameter(
				builder,
				keyOffset,
				FbsValue.Boolean,
				value === true ? 1 : 0
			);
		} else if (typeof value === 'number') {
			// Integer.
			if (value % 1 === 0) {
				const valueOffset = FbsInteger32.createInteger32(builder, value);

				parameterOffset = FbsParameter.createParameter(
					builder,
					keyOffset,
					FbsValue.Integer32,
					valueOffset
				);
			}
			// Float.
			else {
				const valueOffset = FbsDouble.createDouble(builder, value);

				parameterOffset = FbsParameter.createParameter(
					builder,
					keyOffset,
					FbsValue.Double,
					valueOffset
				);
			}
		} else if (typeof value === 'string') {
			const valueOffset = FbsString.createString(
				builder,
				builder.createString(value)
			);

			parameterOffset = FbsParameter.createParameter(
				builder,
				keyOffset,
				FbsValue.String,
				valueOffset
			);
		} else if (Array.isArray(value)) {
			const valueOffset = FbsInteger32Array.createValueVector(builder, value);

			parameterOffset = FbsParameter.createParameter(
				builder,
				keyOffset,
				FbsValue.Integer32Array,
				valueOffset
			);
		} else {
			throw new Error(`invalid parameter type [key:'${key}', value:${value}]`);
		}

		fbsParameters.push(parameterOffset);
	}

	return fbsParameters;
}

export function parseRtcpFeedback(data: FbsRtcpFeedback): RtcpFeedback {
	return {
		type: data.type()!,
		parameter: data.parameter() ?? undefined,
	};
}

export function parseParameters(data: any): any {
	const parameters: any = {};

	for (let i = 0; i < data.parametersLength(); i++) {
		const fbsParameter = data.parameters(i)!;

		switch (fbsParameter.valueType()) {
			case FbsValue.Boolean: {
				const value = new FbsBoolean();

				fbsParameter.value(value);

				parameters[String(fbsParameter.name()!)] = value.value();

				break;
			}

			case FbsValue.Integer32: {
				const value = new FbsInteger32();

				fbsParameter.value(value);

				parameters[String(fbsParameter.name()!)] = value.value();

				break;
			}

			case FbsValue.Double: {
				const value = new FbsDouble();

				fbsParameter.value(value);

				parameters[String(fbsParameter.name()!)] = value.value();

				break;
			}

			case FbsValue.String: {
				const value = new FbsString();

				fbsParameter.value(value);

				parameters[String(fbsParameter.name()!)] = value.value();

				break;
			}

			case FbsValue.Integer32Array: {
				const value = new FbsInteger32Array();

				fbsParameter.value(value);

				parameters[String(fbsParameter.name()!)] = value.valueArray();

				break;
			}
		}
	}

	return parameters;
}

export function parseRtpCodecParameters(
	data: FbsRtpCodecParameters
): RtpCodecParameters {
	const parameters = parseParameters(data);

	let rtcpFeedback: RtcpFeedback[] = [];

	if (data.rtcpFeedbackLength() > 0) {
		rtcpFeedback = utils.parseVector(data, 'rtcpFeedback', parseRtcpFeedback);
	}

	return {
		mimeType: data.mimeType()!,
		payloadType: data.payloadType(),
		clockRate: data.clockRate(),
		channels: data.channels() ?? undefined,
		parameters,
		rtcpFeedback,
	};
}

export function rtpHeaderExtensionUriFromFbs(
	uri: FbsRtpHeaderExtensionUri
): RtpHeaderExtensionUri {
	switch (uri) {
		case FbsRtpHeaderExtensionUri.Mid: {
			return 'urn:ietf:params:rtp-hdrext:sdes:mid';
		}

		case FbsRtpHeaderExtensionUri.RtpStreamId: {
			return 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id';
		}

		case FbsRtpHeaderExtensionUri.RepairRtpStreamId: {
			return 'urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id';
		}

		case FbsRtpHeaderExtensionUri.FrameMarkingDraft07: {
			return 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07';
		}

		case FbsRtpHeaderExtensionUri.FrameMarking: {
			return 'urn:ietf:params:rtp-hdrext:framemarking';
		}

		case FbsRtpHeaderExtensionUri.AudioLevel: {
			return 'urn:ietf:params:rtp-hdrext:ssrc-audio-level';
		}

		case FbsRtpHeaderExtensionUri.VideoOrientation: {
			return 'urn:3gpp:video-orientation';
		}

		case FbsRtpHeaderExtensionUri.TimeOffset: {
			return 'urn:ietf:params:rtp-hdrext:toffset';
		}

		case FbsRtpHeaderExtensionUri.TransportWideCcDraft01: {
			return 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01';
		}

		case FbsRtpHeaderExtensionUri.AbsSendTime: {
			return 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time';
		}

		case FbsRtpHeaderExtensionUri.AbsCaptureTime: {
			return 'http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time';
		}

		case FbsRtpHeaderExtensionUri.PlayoutDelay: {
			return 'http://www.webrtc.org/experiments/rtp-hdrext/playout-delay';
		}
	}
}

export function rtpHeaderExtensionUriToFbs(
	uri: RtpHeaderExtensionUri
): FbsRtpHeaderExtensionUri {
	switch (uri) {
		case 'urn:ietf:params:rtp-hdrext:sdes:mid': {
			return FbsRtpHeaderExtensionUri.Mid;
		}

		case 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id': {
			return FbsRtpHeaderExtensionUri.RtpStreamId;
		}

		case 'urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id': {
			return FbsRtpHeaderExtensionUri.RepairRtpStreamId;
		}

		case 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07': {
			return FbsRtpHeaderExtensionUri.FrameMarkingDraft07;
		}

		case 'urn:ietf:params:rtp-hdrext:framemarking': {
			return FbsRtpHeaderExtensionUri.FrameMarking;
		}

		case 'urn:ietf:params:rtp-hdrext:ssrc-audio-level': {
			return FbsRtpHeaderExtensionUri.AudioLevel;
		}

		case 'urn:3gpp:video-orientation': {
			return FbsRtpHeaderExtensionUri.VideoOrientation;
		}

		case 'urn:ietf:params:rtp-hdrext:toffset': {
			return FbsRtpHeaderExtensionUri.TimeOffset;
		}

		case 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01': {
			return FbsRtpHeaderExtensionUri.TransportWideCcDraft01;
		}

		case 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time': {
			return FbsRtpHeaderExtensionUri.AbsSendTime;
		}

		case 'http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time': {
			return FbsRtpHeaderExtensionUri.AbsCaptureTime;
		}

		case 'http://www.webrtc.org/experiments/rtp-hdrext/playout-delay': {
			return FbsRtpHeaderExtensionUri.PlayoutDelay;
		}

		default: {
			throw new TypeError(`invalid RtpHeaderExtensionUri: ${uri}`);
		}
	}
}

export function parseRtpHeaderExtensionParameters(
	data: FbsRtpHeaderExtensionParameters
): RtpHeaderExtensionParameters {
	return {
		uri: rtpHeaderExtensionUriFromFbs(data.uri()),
		id: data.id(),
		encrypt: data.encrypt(),
		parameters: parseParameters(data),
	};
}

export function parseRtpEncodingParameters(
	data: FbsRtpEncodingParameters
): RtpEncodingParameters {
	return {
		ssrc: data.ssrc() ?? undefined,
		rid: data.rid() ?? undefined,
		codecPayloadType:
			data.codecPayloadType() !== null ? data.codecPayloadType()! : undefined,
		rtx: data.rtx() ? { ssrc: data.rtx()!.ssrc()! } : undefined,
		dtx: data.dtx(),
		scalabilityMode: data.scalabilityMode() ?? undefined,
		maxBitrate: data.maxBitrate() !== null ? data.maxBitrate()! : undefined,
	};
}

export function parseRtpParameters(data: FbsRtpParameters): RtpParameters {
	const codecs = utils.parseVector(data, 'codecs', parseRtpCodecParameters);

	let headerExtensions: RtpHeaderExtensionParameters[] = [];

	if (data.headerExtensionsLength() > 0) {
		headerExtensions = utils.parseVector(
			data,
			'headerExtensions',
			parseRtpHeaderExtensionParameters
		);
	}

	let encodings: RtpEncodingParameters[] = [];

	if (data.encodingsLength() > 0) {
		encodings = utils.parseVector(
			data,
			'encodings',
			parseRtpEncodingParameters
		);
	}

	let rtcp: RtcpParameters | undefined;

	if (data.rtcp()) {
		const fbsRtcp = data.rtcp()!;

		rtcp = {
			cname: fbsRtcp.cname() ?? undefined,
			reducedSize: fbsRtcp.reducedSize(),
		};
	}

	return {
		mid: data.mid() ?? undefined,
		codecs,
		headerExtensions,
		encodings,
		rtcp,
	};
}
