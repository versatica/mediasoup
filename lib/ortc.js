const utils = require('./utils');
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
			.find((supportedCodec) => matchCodecs(mediaCodec, supportedCodec));

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
 * Checks whether aCaps is a valid subset of bCaps. It throws otherwise.
 *
 * @param {RTCRtpCapabilities} aCaps
 * @param {RTCRtpCapabilities} bCaps
 */
exports.assertCapabilitiesSubset = function(aCaps, bCaps)
{
	for (const aCodec of aCaps.codecs)
	{
		const matchedBCodec = bCaps.codecs
			.find((bCodec) => (
				aCodec.preferredPayloadType === bCodec.preferredPayloadType &&
				matchCodecs(aCodec, bCodec)
			));

		if (!matchedBCodec)
		{
			throw new Error(
				'unsupported codec ' +
				`[name:${aCodec.name}, preferredPayloadType:${aCodec.preferredPayloadType}]`);
		}
	}

	for (const aExt of aCaps.headerExtensions)
	{
		const matchedBExt = bCaps.headerExtensions
			.find((bExt) => (
				aExt.preferredId === bExt.preferredId &&
				matchHeaderExtensions(aExt, bExt)
			));

		if (!matchedBExt)
		{
			throw new Error(
				`unsupported header extension [kind:${aExt.kind}, uri:${aExt.uri}]`);
		}
	}
};

/**
 * Generate a mapping of codec payload types and header extension ids that maps
 * the RTP parameters of a Producer and the associated values in the RTP
 * capabilities of the Room.
 * It may throw if invalid or non supported RTP parameters are given.
 *
 * @param {RTCRtpParameters} params
 * @param {RTCRtpCapabilities} caps
 *
 * @return {Object}
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
			.find((capCodec) => matchCodecs(codec, capCodec));

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
			.find((capCodec) => (
				capCodec.name.toLowerCase() === 'rtx' &&
				capCodec.parameters.apt === capMediaCodec.preferredPayloadType
			));

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
			.find((capExt) => matchHeaderExtensions(ext, capExt));

		if (!matchedCapExt)
		{
			throw new Error(
				`unsupported header extensions [uri:"${ext.uri}", id:${ext.id}]`);
		}

		mapHeaderExtensionIds.set(ext.id, matchedCapExt.preferredId);
	}

	return {
		mapCodecPayloadTypes,
		mapHeaderExtensionIds
	};
};

/**
 * Generate RTP parameters for Consumers given the RTP parameters of a Producer
 * and the RTP capabilities of the Room.
 * It may throw if invalid or non supported RTP parameters are given.
 *
 * @param {RTCRtpParameters} params
 * @param {RTCRtpCapabilities} caps
 * @param {Object} rtpMapping - As generated by getProducerRtpParametersMapping().
 *
 * @return {RTCRtpParameters}
 */
exports.getConsumableRtpParameters = function(params, caps, rtpMapping)
{
	const { mapCodecPayloadTypes, mapHeaderExtensionIds } = rtpMapping;
	const consumableParams =
	{
		muxId            : params.muxId,
		codecs           : [],
		headerExtensions : [],
		encodings        : params.encodings,
		rtcp             : params.rtcp
	};

	for (const codec of params.codecs)
	{
		if (codec.name.toLowerCase() === 'rtx')
			continue;

		const consumableCodecPt = mapCodecPayloadTypes.get(codec.payloadType);

		if (!consumableCodecPt)
		{
			throw new Error(
				`codec payloadType mapping not found [name:${codec.name}]`);
		}

		const matchedCapCodec = caps.codecs
			.find((capCodec) => capCodec.preferredPayloadType === consumableCodecPt);

		if (!matchedCapCodec)
			throw new Error(`capabilities codec not found [name:${codec.name}]`);

		const consumableCodec =
		{
			name         : matchedCapCodec.name,
			mimeType     : matchedCapCodec.mimeType,
			clockRate    : matchedCapCodec.clockRate,
			payloadType  : matchedCapCodec.preferredPayloadType,
			channels     : matchedCapCodec.channels,
			rtcpFeedback : matchedCapCodec.rtcpFeedback,
			parameters   : matchedCapCodec.parameters
		};

		if (!consumableCodec.channels)
			delete consumableCodec.channels;

		consumableParams.codecs.push(consumableCodec);

		const consumableCapRtxCodec = caps.codecs
			.find((capRtxCodec) => (
				capRtxCodec.name.toLowerCase() === 'rtx' &&
				capRtxCodec.parameters.apt === consumableCodec.payloadType
			));

		if (consumableCapRtxCodec)
		{
			const consumableRtxCodec =
			{
				name         : consumableCapRtxCodec.name,
				mimeType     : consumableCapRtxCodec.mimeType,
				clockRate    : consumableCapRtxCodec.clockRate,
				payloadType  : consumableCapRtxCodec.preferredPayloadType,
				channels     : consumableCapRtxCodec.channels,
				rtcpFeedback : consumableCapRtxCodec.rtcpFeedback,
				parameters   : consumableCapRtxCodec.parameters
			};

			if (!consumableRtxCodec.channels)
				delete consumableRtxCodec.channels;

			consumableParams.codecs.push(consumableRtxCodec);
		}
	}

	for (const ext of params.headerExtensions)
	{
		const consumableExtId = mapHeaderExtensionIds.get(ext.id);

		if (!consumableExtId)
		{
			throw new Error(
				`extension header id mapping not found [uri:${ext.uri}]`);
		}

		const matchedCapExt = caps.headerExtensions
			.find((capExt) => capExt.preferredId === consumableExtId);

		if (!matchedCapExt)
			throw new Error(`capabilities header extension not found [uri:${ext.uri}]`);

		const consumableExt =
		{
			uri : matchedCapExt.uri,
			id  : matchedCapExt.preferredId
		};

		consumableParams.headerExtensions.push(consumableExt);
	}

	return consumableParams;
};

/**
 * Generate RTP parameters for a specific Consumer.
 *
 * NOTE: It's up to the remote Consumer to check the codecs and decide whether it
 * can enable this Consumer or not.
 *
 * @param {RTCRtpParameters} consumableRtpParameters - Consumable RTP parameters.
 * @param {RTCRtpCapabilities} rtpCapabilities - Peer RTP capabilities.
 *
 * @return {RTCRtpParameters}
 */
exports.getConsumerRtpParameters = function(consumableRtpParameters, rtpCapabilities)
{
	const consumerParams =
	{
		muxId            : consumableRtpParameters.muxId,
		codecs           : [],
		headerExtensions : [],
		encodings        : [],
		rtcp             : consumableRtpParameters.rtcp
	};

	const consumableCodecs = utils.cloneObject(consumableRtpParameters.codecs);
	let rtxSupported = false;

	for (const codec of consumableCodecs)
	{
		const matchedCapCodec = rtpCapabilities.codecs
			.find((capCodec) => capCodec.preferredPayloadType === codec.payloadType);

		if (!matchedCapCodec)
			continue;

		// Reduce RTCP feedbacks.
		codec.rtcpFeedback = matchedCapCodec.rtcpFeedback;

		consumerParams.codecs.push(codec);

		if (!rtxSupported && codec.name.toLowerCase() === 'rtx')
			rtxSupported = true;
	}

	consumerParams.headerExtensions = consumableRtpParameters.headerExtensions
		.filter((ext) =>
			rtpCapabilities.headerExtensions
				.some((capExt) => capExt.preferredId === ext.id)
		);

	const consumerEncoding =
	{
		ssrc : utils.randomNumber()
	};

	if (rtxSupported)
	{
		consumerEncoding.rtx =
		{
			ssrc : utils.randomNumber()
		};
	}

	consumerParams.encodings.push(consumerEncoding);

	return consumerParams;
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

function matchCodecs(aCodec, bCodec)
{
	const aName = aCodec.name.toLowerCase();
	const bName = bCodec.name.toLowerCase();

	if (aCodec.kind && bCodec.kind && aCodec.kind !== bCodec.kind)
		return false;

	if (aName !== bName)
		return false;

	if (aCodec.clockRate !== bCodec.clockRate)
		return false;

	if (aCodec.channels !== bCodec.channels)
		return false;

	// Match H264 parameters.
	if (aName === 'h264')
	{
		const aPacketizationMode = (aCodec.parameters || {})['packetization-mode'] || 0;
		const bPacketizationMode = (bCodec.parameters || {})['packetization-mode'] || 0;

		if (aPacketizationMode !== bPacketizationMode)
			return false;
	}

	return true;
}

function matchHeaderExtensions(aExt, bExt)
{
	if (aExt.kind && bExt.kind && aExt.kind !== bExt.kind)
		return false;

	if (aExt.uri !== bExt.uri)
		return false;

	return true;
}
