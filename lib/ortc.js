const utils = require('./utils');
const { UnsupportedError } = require('./errors');
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
 * Generate RTP capabilities for the Router based on the given media codecs and
 * mediasoup supported RTP capabilities.
 *
 * @param {array<RTCRtpCodecCapability>} mediaCodecs
 *
 * @returns {RTCRtpCapabilities}
 * @throws {UnsupportedError} if codec not supported.
 * @throws {Error}
 */
exports.generateRouterRtpCapabilities = function(mediaCodecs)
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
		{
			throw new UnsupportedError(
				`media codec not supported [name:${mediaCodec.name}]`);
		}

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

		// Make rtcpFeedback an array.
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
				rtcpFeedback         : [],
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
 * Checks whether aCaps is a valid subset of bCaps and throws if not.
 *
 * @param {RTCRtpCapabilities} aCaps
 * @param {RTCRtpCapabilities} bCaps
 *
 * @throws {UnsupportedError} if codec or header extension not supported.
 */
exports.assertCapabilities = function(aCaps, bCaps)
{
	for (const aCodec of aCaps.codecs)
	{
		const matched = bCaps.codecs
			.some((bCodec) => (
				aCodec.preferredPayloadType === bCodec.preferredPayloadType &&
				matchCodecs(aCodec, bCodec)
			));

		if (!matched)
		{
			throw new UnsupportedError(
				`unsupported codec [name:${aCodec.name}, preferredPayloadType:${aCodec.preferredPayloadType}]`);
		}
	}

	for (const aExt of aCaps.headerExtensions || [])
	{
		const matched = bCaps.headerExtensions
			.some((bExt) => (
				aExt.preferredId === bExt.preferredId &&
				matchHeaderExtensions(aExt, bExt)
			));

		if (!matched)
		{
			throw new UnsupportedError(
				`unsupported header extension [kind:${aExt.kind}, uri:${aExt.uri}]`);
		}
	}
};

/**
 * Get a mapping of the codec payload, RTP header extensions and encodings from
 * the given Producer RTP parameters to the values expected by the Router.
 *
 * It may throw if invalid or non supported RTP parameters are given.
 *
 * @param {RTCRtpParameters} params
 * @param {RTCRtpCapabilities} caps
 *
 * @returns {Object} with codecs, headerExtensions and encodings arrays of objects.
 * @throws {TypeError} if wrong arguments.
 * @throws {UnsupportedError} if codec not supported.
 * @throws {Error}
 */
exports.getProducerRtpParametersMapping = function(params, caps)
{
	const mapping =
	{
		codecs           : [],
		headerExtensions : [],
		encodings        : []
	};

	// Best effort to detect RTX support when there is RID so no
	// encoding.rtx.ssrc.
	let hasRtx = false;

	// Match parameters media codecs to capabilities media codecs.

	const codecToCapCodec = new Map();

	for (const codec of params.codecs)
	{
		if (codec.name.toLowerCase() === 'rtx')
			continue;

		// Search for the same media codec in capabilities.
		const matchedCapCodec = caps.codecs
			.find((capCodec) => matchCodecs(codec, capCodec));

		if (!matchedCapCodec)
		{
			throw new UnsupportedError(
				`unsupported codec [name:${codec.name}, payloadType:${codec.payloadType}]`);
		}

		codecToCapCodec.set(codec, matchedCapCodec);
	}

	// Match parameters RTX codecs to capabilities RTX codecs.
	for (const codec of params.codecs)
	{
		if (codec.name.toLowerCase() !== 'rtx')
			continue;
		else if (typeof codec.parameters !== 'object')
			throw TypeError('missing parameters in RTX codec');

		// Search for the associated media codec.
		const associatedMediaCodec = params.codecs
			.find((mediaCodec) => mediaCodec.payloadType === codec.parameters.apt);

		if (!associatedMediaCodec)
		{
			throw new TypeError(
				`missing media codec found for RTX PT ${codec.payloadType}`);
		}

		const capMediaCodec = codecToCapCodec.get(associatedMediaCodec);

		// Ensure that the capabilities media codec has a RTX codec.
		const associatedCapRtxCodec = caps.codecs
			.find((capCodec) => (
				capCodec.name.toLowerCase() === 'rtx' &&
				capCodec.parameters.apt === capMediaCodec.preferredPayloadType
			));

		if (!associatedCapRtxCodec)
		{
			throw new UnsupportedError(
				`no RTX codec for capability codec PT ${capMediaCodec.preferredPayloadType}`);
		}

		codecToCapCodec.set(codec, associatedCapRtxCodec);

		hasRtx = true;
	}

	// Generate codecs mapping.
	for (const [ codec, capCodec ] of codecToCapCodec)
	{
		mapping.codecs.push(
			{
				payloadType       : codec.payloadType,
				mappedPayloadType : capCodec.preferredPayloadType
			});
	}

	// Generate header extensions mapping.
	for (const ext of (params.headerExtensions || []))
	{
		const matchedCapExt = caps.headerExtensions
			.find((capExt) => matchHeaderExtensions(ext, capExt));

		if (!matchedCapExt)
		{
			throw new UnsupportedError(
				`unsupported header extensions [uri:"${ext.uri}", id:${ext.id}]`);
		}

		mapping.headerExtensions.push(
			{
				id       : ext.id,
				mappedId : matchedCapExt.preferredId
			});
	}

	// Generate encodings mapping.
	for (const encoding of (params.encodings || []))
	{
		const mappedSsrc = utils.generateRandomNumber();
		const mappedRtxSsrc = hasRtx ? utils.generateRandomNumber() : undefined;
		const mappedEncoding = {};

		if (encoding.rid)
			mappedEncoding.rid = encoding.rid;
		if (encoding.ssrc)
			mappedEncoding.ssrc = encoding.ssrc;
		if (hasRtx && encoding.rtx && encoding.rtx.ssrc)
			mappedEncoding.rtxSsrc = encoding.rtx.ssrc;
		if (mappedSsrc)
			mappedEncoding.mappedSsrc = mappedSsrc;
		if (mappedRtxSsrc)
			mappedEncoding.mappedRtxSsrc = mappedRtxSsrc;

		mapping.encodings.push(mappedEncoding);
	}

	return mapping;
};

/**
 * Generate RTP parameters for Consumers given the RTP parameters of a Producer
 * and the RTP capabilities of the Router.
 *
 * @param {String} kind
 * @param {RTCRtpParameters} params
 * @param {RTCRtpCapabilities} caps
 * @param {Object} rtpMapping - As generated by getProducerRtpParametersMapping().
 *
 * @returns {RTCRtpParameters}
 * @throws {TypeError} if invalid or non supported RTP parameters are given.
 */
exports.getConsumableRtpParameters = function(kind, params, caps, mapping)
{
	const consumableParams =
	{
		codecs           : [],
		headerExtensions : [],
		encodings        : [],
		rtcp             : {}
	};

	for (const codec of params.codecs)
	{
		if (codec.name.toLowerCase() === 'rtx')
			continue;

		const consumableCodecPt = mapping.codecs
			.find((entry) => entry.payloadType === codec.payloadType)
			.mappedPayloadType;

		const matchedCapCodec = caps.codecs
			.find((capCodec) => capCodec.preferredPayloadType === consumableCodecPt);

		const consumableCodec =
		{
			name         : matchedCapCodec.name,
			mimeType     : matchedCapCodec.mimeType,
			clockRate    : matchedCapCodec.clockRate,
			payloadType  : matchedCapCodec.preferredPayloadType,
			channels     : matchedCapCodec.channels,
			rtcpFeedback : matchedCapCodec.rtcpFeedback,
			parameters   : codec.parameters // Keep the Producer parameters.
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

	for (const capExt of caps.headerExtensions)
	{
		if (
			capExt.kind !== kind ||
			capExt.uri === 'urn:ietf:params:rtp-hdrext:sdes:mid' ||
			capExt.uri === 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id'
		)
		{
			continue;
		}

		const consumableExt =
		{
			uri : capExt.uri,
			id  : capExt.preferredId
		};

		consumableParams.headerExtensions.push(consumableExt);
	}

	for (const entry of mapping.encodings)
	{
		const consumableEncoding =
		{
			ssrc : entry.mappedSsrc
		};

		if (entry.mappedRtxSsrc)
			consumableEncoding.rtx = { ssrc: entry.mappedRtxSsrc };

		consumableParams.encodings.push(consumableEncoding);
	}

	consumableParams.rtcp =
	{
		cname       : params.rtcp.cname,
		reducedSize : true,
		mux         : true
	};

	return consumableParams;
};

/**
 * Generate RTP parameters for a specific Consumer.
 * It reduces encodings to just one and takes into account given RTP capabilities
 * to reduce codecs, codecs' RTCP feedback and header extensions, and also enables
 * or disabled RTX.
 *
 * NOTE: It's up to the remote Consumer to check the codecs and decide whether it
 * can consume or not the corresponding Producer.
 *
 * @param {RTCRtpParameters} consumableParams - Consumable RTP parameters.
 * @param {RTCRtpCapabilities} caps - Remote RTP capabilities.
 *
 * @returns {RTCRtpParameters}
 * @throws {UnsupportedError} if codecs are not compatible.
 */
exports.getConsumerRtpParameters = function(consumableParams, caps)
{
	const consumerParams =
	{
		codecs           : [],
		headerExtensions : [],
		encodings        : [],
		rtcp             : consumableParams.rtcp
	};

	const consumableCodecs = utils.clone(consumableParams.codecs);
	let rtxSupported = false;

	for (const codec of consumableCodecs)
	{
		const matchedCapCodec = caps.codecs
			.find((capCodec) => capCodec.preferredPayloadType === codec.payloadType);

		if (!matchedCapCodec)
			continue;

		// Reduce RTCP feedbacks.
		codec.rtcpFeedback = matchedCapCodec.rtcpFeedback || [];

		consumerParams.codecs.push(codec);

		if (!rtxSupported && codec.name.toLowerCase() === 'rtx')
			rtxSupported = true;
	}

	// Ensure there is at least one media codec.
	if (
		consumerParams.codecs.length === 0 ||
		consumerParams.codecs[0].name === 'rtx'
	)
	{
		throw new UnsupportedError('no compatible media codecs');
	}

	consumerParams.headerExtensions = consumableParams.headerExtensions
		.filter((ext) => (
			(caps.headerExtensions || [])
				.some((capExt) => capExt.preferredId === ext.id)
		));

	const consumerEncoding =
	{
		ssrc : utils.generateRandomNumber()
	};

	if (rtxSupported)
		consumerEncoding.rtx = { ssrc: utils.generateRandomNumber() };

	consumerParams.encodings.push(consumerEncoding);

	// Copy verbatim.
	consumerParams.rtcp = consumableParams.rtcp;

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
		throw new TypeError('invalid RTCRtpCodecCapability');
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

	if (
		(aCodec.kind === 'audio' || bCodec.kind === 'audio') &&
		(
			(aCodec.channels !== undefined && aCodec.channels !== 1) ||
			(bCodec.channels !== undefined && bCodec.channels !== 1)
		) &&
		aCodec.channels !== bCodec.channels
	)
	{
		return false;
	}

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
