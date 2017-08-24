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
		if (!isValidCodecCapability(mediaCodec))
			throw new TypeError('invalid mediaCodec');

		const matchedSupportedCodec = supportedCodecs
			.find((supportedCodec) =>
			{
				return matchCodecCapabilities(mediaCodec, supportedCodec);
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
					matchCodecCapabilities(codecA, codecB)
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

function isValidCodecCapability(codec)
{
	return (
		(typeof codec === 'object' && !Array.isArray(codec)) &&
		(typeof codec.kind === 'string' && codec.kind) &&
		(typeof codec.name === 'string' && codec.name) &&
		(typeof codec.clockRate === 'number' && codec.clockRate)
	);
}

function matchCodecCapabilities(codecA, codecB)
{
	if (codecA.kind !== codecB.kind)
		return false;

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

function matchHeaderExtensions(extA, extB)
{
	return (
		extA.kind === extB.kind &&
		extA.uri === extB.uri
	);
}
