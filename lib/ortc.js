'use strict';

const supportedRtpCapabilities = require('./supportedRtpCapabilities');

const DYNAMIC_PAYLOAD_TYPES =
[
	100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
	116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 96, 97, 98, 99, 77,
	78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 35, 36,
	37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
	57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71
];

/**
 * Generate RTP capabilities for the room based on the given media codecs and
 * mediasoup supported RTP capabilities. It may throw.
 *
 * @param {array<RoomMediaCodec>]} mediaCodecs
 *
 * @return {RTCRtpCapabilities}
 */
exports.generateRoomRtpCapabilities = function(mediaCodecs)
{
	if (!Array.isArray(mediaCodecs))
		throw new TypeError('mediaCodecs must be an Array');
	else if (mediaCodecs.length === 0)
		throw new TypeError('mediaCodecs cannot be empty');

	const supportedCodecs = supportedRtpCapabilities.codecs;
	const caps =
	{
		codecs           : [],
		headerExtensions : supportedRtpCapabilities.headerExtensions,
		fecMechanisms    : supportedRtpCapabilities.fecMechanisms
	};

	let dynamicPayloadTypeIdx = 0;

	for (const mediaCodec of mediaCodecs)
	{
		assertCodecCapability(mediaCodec);

		const matchedSupportedCodec = supportedCodecs
			.find((supportedCodec) =>
			{
				return matchCodec(mediaCodec, supportedCodec);
			});

		if (!matchedSupportedCodec)
			throw new Error(`RoomMediaCodec not supported [name:${mediaCodec.name}]`);

		// Clone the supported codec.
		const codec = Object.assign({}, matchedSupportedCodec);

		// Normalize channels.
		if (codec.kind !== 'audio')
			delete codec.channels;
		else if (!codec.channels)
			codec.channels = 1;

		// Merge the media codec parameters.
		codec.parameters =
			Object.assign({}, codec.parameters, mediaCodec.parameters);

		// Ensure rtcpFeedback is an Array.
		codec.rtcpFeedback = codec.rtcpFeedback || [];

		// Assign a payload type.
		if (typeof codec.preferredPayloadType !== 'number')
		{
			const pt = DYNAMIC_PAYLOAD_TYPES[dynamicPayloadTypeIdx++];

			if (!pt)
				throw new Error('cannot allocate more dynamic codec payload types');

			codec.preferredPayloadType = pt;
		}

		// Append to the codec list.
		caps.codecs.push(codec);

		// Add a RTX video codec if video.
		if (codec.kind === 'video')
		{
			const pt = DYNAMIC_PAYLOAD_TYPES[dynamicPayloadTypeIdx++];

			if (!pt)
				throw new Error('cannot allocate more dynamic codec payload types');

			const rtxCodec =
			{
				kind                 : codec.kind,
				name                 : 'rtx',
				mimeType             : `${codec.kind}/rtx`,
				preferredPayloadType : pt,
				clockRate            : codec.clockRate,
				parameters           :
				{
					apt : codec.preferredPayloadType
				}
			};

			// Append to the codec list.
			caps.codecs.push(rtxCodec);
		}
	}

	return caps;
};

/**
 * Checks whether capsA is a valid subset of capsB. It throws otherwise.
 *
 * @param {RTCRtpCapabilities} capsA
 * @param {RTCRtpCapabilities} capsB
 */
exports.assertCapabilitiesSubset = function(capsA, capsB)
{
	for (const codecA of capsA.codecs)
	{
		const matchedCodecB = capsB.codecs
			.find((codecB) =>
			{
				return (
					codecA.preferredPayloadType === codecB.preferredPayloadType &&
					matchCodec(codecA, codecB)
				);
			});

		if (!matchedCodecB)
		{
			throw new Error(
				'unsupported codec ' +
				`[name:${codecA.name}, preferredPayloadType:${codecA.preferredPayloadType}]`);
		}
	}

	for (const extA of capsA.headerExtensions)
	{
		const matchedExtB = capsB.headerExtensions
			.find((extB) =>
			{
				return (
					extA.preferredId === extB.preferredId &&
					matchHeaderExtensions(extA, extB)
				);
			});

		if (!matchedExtB)
		{
			throw new Error(
				`unsupported header extension [kind:${extA.kind}, uri:${extA.uri}]`);
		}
	}
};

/**
 * Generate an Object with RTP parameters and RTP mapping.
 * It may throw if invalid or non supported RTP parameters are given.
 *
 * @param {RTCRtpParameters} params
 * @param {RTCRtpCapabilities} caps
 *
 * @return {Object}
 *
 * TODO: This method should also reduce encodings in case of simulcast/SVC
 * so the resulting consumableRtpParameters are suitable for a Consumer.
 */
exports.getProducerRtpParametersMapping = function(params, caps)
{
	const codecToCapCodec = new Map();

	// Match parameters media codecs to capabilities media codecs.
	for (const codec of params.codecs)
	{
		if (codec.name.toLowerCase() === 'rtx')
			continue;

		// Search for the same media codec in capabilities.
		const matchedCapCodec = caps.codecs
			.find((capCodec) => matchCodec(codec, capCodec, true));

		if (!matchedCapCodec)
		{
			throw new Error(
				'unsupported codec ' +
				`[name:${codec.name}, payloadType:${codec.payloadType}]`);
		}

		codecToCapCodec.set(codec, matchedCapCodec);
	}

	// Match parameters RTX codecs to capabilities RTX codecs.
	for (const codec of params.codecs)
	{
		if (codec.name.toLowerCase() !== 'rtx')
			continue;

		// Search for the associated media codec in parameters.
		const associatedMediaCodec = params.codecs
			.find((mediaCodec) => mediaCodec.payloadType === codec.parameters.apt);

		const capMediaCodec = codecToCapCodec.get(associatedMediaCodec);

		if (!capMediaCodec)
		{
			throw new Error(
				`no media codec found for RTX PT ${codec.payloadType}`);
		}

		// Ensure that the capabilities media codec has a RTX codec.
		const associatedCapRtxCodec = caps.codecs
			.find((capCodec) =>
			{
				return (
					capCodec.name.toLowerCase() === 'rtx' &&
					capCodec.parameters.apt === capMediaCodec.preferredPayloadType
				);
			});

		if (!associatedCapRtxCodec)
		{
			throw new Error(
				'no RTX codec found in capabilities ' +
				`[capabilities codec PT:${capMediaCodec.preferredPayloadType}]`);
		}

		codecToCapCodec.set(codec, associatedCapRtxCodec);
	}

	const mapCodecPayloadTypes = new Map();

	for (const [ codec, capCodec ] of codecToCapCodec)
	{
		mapCodecPayloadTypes.set(codec.payloadType, capCodec.preferredPayloadType);
	}

	const mapHeaderExtensionIds = new Map();

	for (const ext of params.headerExtensions)
	{
		const matchedCapExt = caps.headerExtensions
			.find((capExt) => matchHeaderExtensions(ext, capExt, true));

		if (!matchedCapExt)
		{
			throw new Error(
				`unsupported header extensions [uri:"${ext.uri}", id:${ext.id}]`);
		}

		mapHeaderExtensionIds.set(ext.id, matchedCapExt.preferredId);
	}

	return {
		codecPayloadTypes  : Array.from(mapCodecPayloadTypes),
		headerExtensionIds : Array.from(mapHeaderExtensionIds)
	};
};

function assertCodecCapability(codec)
{
	const valid =
		(typeof codec === 'object' && !Array.isArray(codec)) &&
		(typeof codec.kind === 'string' && codec.kind) &&
		(typeof codec.name === 'string' && codec.name) &&
		(typeof codec.clockRate === 'number' && codec.clockRate);

	if (!valid)
		throw new TypeError('invalid RTCCodecCapability');
}

function matchCodec(codecA, codecB, leftIsParam = false)
{
	if (!leftIsParam)
	{
		if (codecA.kind !== codecB.kind)
			return false;
	}

	if (codecA.name.toLowerCase() !== codecB.name.toLowerCase())
		return false;

	if (codecA.clockRate !== codecB.clockRate)
		return false;

	if (
		(codecA.channels > 1 || codecB.channels > 1) &&
		codecA.channels !== codecB.channels)
	{
		return false;
	}

	// TODO: H264 parameters.

	return true;
}

function matchHeaderExtensions(extA, extB, leftIsParam = false)
{
	if (!leftIsParam)
	{
		if (extA.kind !== extB.kind)
			return false;
	}

	if (extA.uri !== extB.uri)
		return false;

	return true;
}
