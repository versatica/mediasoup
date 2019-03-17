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
				`media codec not supported [mimeType:${mediaCodec.mimeType}]`);
		}

		// Clone the supported codec.
		const codec = utils.clone(matchedSupportedCodec);

		// Normalize channels.
		if (codec.kind !== 'audio')
			delete codec.channels;
		else if (!codec.channels)
			codec.channels = 1;

		// Merge the media codec parameters.
		codec.parameters = { ...codec.parameters, ...mediaCodec.parameters };

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
				`unsupported codec [mimeType:${aCodec.mimeType}, preferredPayloadType:${aCodec.preferredPayloadType}]`);
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
 *   Each codec has payloadType and mappedPayloadType.
 *   Each headerExtension has id and mappedId.
 *   Each encoding may have rid and/or ssrc, and mandatory mappedSsrc.
 * @throws {TypeError} if wrong arguments.
 * @throws {UnsupportedError} if codec not supported.
 * @throws {Error}
 */
exports.getProducerRtpParametersMapping = function(params, caps)
{
	const rtpMapping =
	{
		codecs           : [],
		headerExtensions : [],
		encodings        : []
	};

	// Match parameters media codecs to capabilities media codecs.
	const codecToCapCodec = new Map();

	for (const codec of params.codecs || [])
	{
		assertCodecParameters(codec);

		if (/.+\/rtx$/i.test(codec.mimeType))
			continue;

		// Search for the same media codec in capabilities.
		const matchedCapCodec = caps.codecs
			.find((capCodec) => matchCodecs(codec, capCodec));

		if (!matchedCapCodec)
		{
			throw new UnsupportedError(
				`unsupported codec [mimeType:${codec.mimeType}, payloadType:${codec.payloadType}]`);
		}

		codecToCapCodec.set(codec, matchedCapCodec);
	}

	// Match parameters RTX codecs to capabilities RTX codecs.
	for (const codec of params.codecs || [])
	{
		if (!/.+\/rtx$/i.test(codec.mimeType))
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
				/.+\/rtx$/i.test(capCodec.mimeType) &&
				capCodec.parameters.apt === capMediaCodec.preferredPayloadType
			));

		if (!associatedCapRtxCodec)
		{
			throw new UnsupportedError(
				`no RTX codec for capability codec PT ${capMediaCodec.preferredPayloadType}`);
		}

		codecToCapCodec.set(codec, associatedCapRtxCodec);
	}

	// Generate codecs mapping.
	for (const [ codec, capCodec ] of codecToCapCodec)
	{
		rtpMapping.codecs.push(
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

		rtpMapping.headerExtensions.push(
			{
				id       : ext.id,
				mappedId : matchedCapExt.preferredId
			});
	}

	// Generate encodings mapping.
	for (const encoding of (params.encodings || []))
	{
		const mappedSsrc = utils.generateRandomNumber();
		const mappedEncoding = {};

		if (encoding.rid)
			mappedEncoding.rid = encoding.rid;
		if (encoding.ssrc)
			mappedEncoding.ssrc = encoding.ssrc;
		if (mappedSsrc)
			mappedEncoding.mappedSsrc = mappedSsrc;

		rtpMapping.encodings.push(mappedEncoding);
	}

	return rtpMapping;
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
exports.getConsumableRtpParameters = function(kind, params, caps, rtpMapping)
{
	const consumableParams =
	{
		codecs           : [],
		headerExtensions : [],
		encodings        : [],
		rtcp             : {}
	};

	for (const codec of params.codecs || [])
	{
		assertCodecParameters(codec);

		if (/.+\/rtx$/i.test(codec.mimeType))
			continue;

		const consumableCodecPt = rtpMapping.codecs
			.find((entry) => entry.payloadType === codec.payloadType)
			.mappedPayloadType;

		const matchedCapCodec = caps.codecs
			.find((capCodec) => capCodec.preferredPayloadType === consumableCodecPt);

		const consumableCodec =
		{
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
				/.+\/rtx$/i.test(capRtxCodec.mimeType) &&
				capRtxCodec.parameters.apt === consumableCodec.payloadType
			));

		if (consumableCapRtxCodec)
		{
			const consumableRtxCodec =
			{
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
			capExt.uri === 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id' ||
			capExt.uri === 'urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id'
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

	// Clone Producer encodings since we'll mangle them.
	const consumableEncodings = utils.clone(params.encodings);

	for (let i = 0; i < consumableEncodings.length; ++i)
	{
		const consumableEncoding = consumableEncodings[i];
		const { mappedSsrc } = rtpMapping.encodings[i];

		// Remove useless fields.
		delete consumableEncoding.rid;
		delete consumableEncoding.rtx;
		delete consumableEncoding.codecPayloadType;

		// Set the mapped ssrc.
		consumableEncoding.ssrc = mappedSsrc;

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
 * Check whether the given RTP capabilities can consume the given Producer.
 *
 * @param {RTCRtpParameters} consumableParams - Consumable RTP parameters.
 * @param {RTCRtpCapabilities} caps - Remote RTP capabilities.
 *
 * @returns {RTCRtpParameters}
 * @throws {TypeError} if wrong arguments.
 */
exports.canConsume = function(consumableParams, caps)
{
	const matchingCodecs = [];

	for (const capCodec of caps.codecs || [])
	{
		assertCodecCapability(capCodec);
	}

	for (const codec of consumableParams.codecs)
	{
		const matchedCapCodec = caps.codecs
			.find((capCodec) => matchCodecs(capCodec, codec));

		if (!matchedCapCodec)
			continue;

		matchingCodecs.push(codec);
	}

	// Ensure there is at least one media codec.
	if (
		matchingCodecs.length === 0 ||
		/.+\/rtx$/i.test(matchingCodecs[0].mimeType)
	)
	{
		return false;
	}

	return true;
};

/**
 * Generate RTP parameters for a specific Consumer.
 *
 * It reduces encodings to just one and takes into account given RTP capabilities
 * to reduce codecs, codecs' RTCP feedback and header extensions, and also enables
 * or disabled RTX.
 *
 * @param {RTCRtpParameters} consumableParams - Consumable RTP parameters.
 * @param {RTCRtpCapabilities} caps - Remote RTP capabilities.
 *
 * @returns {RTCRtpParameters}
 * @throws {TypeError} if wrong arguments.
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

	for (const capCodec of caps.codecs || [])
	{
		assertCodecCapability(capCodec);
	}

	const consumableCodecs = utils.clone(consumableParams.codecs || []);
	let rtxSupported = false;

	for (const codec of consumableCodecs)
	{
		const matchedCapCodec = caps.codecs
			.find((capCodec) => matchCodecs(capCodec, codec));

		if (!matchedCapCodec)
			continue;

		// Reduce RTCP feedbacks.
		codec.rtcpFeedback = matchedCapCodec.rtcpFeedback || [];

		consumerParams.codecs.push(codec);

		if (!rtxSupported && /.+\/rtx$/i.test(codec.mimeType))
			rtxSupported = true;
	}

	// Ensure there is at least one media codec.
	if (
		consumerParams.codecs.length === 0 ||
		/.+\/rtx$/i.test(consumerParams.codecs[0].mimeType)
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

/**
 * Generate RTP parameters for a pipe Consumer.
 *
 * It keeps all original consumable encodings, removes RTX support and also
 * other features such as NACK.
 *
 * @param {RTCRtpParameters} consumableParams - Consumable RTP parameters.
 *
 * @returns {RTCRtpParameters}
 * @throws {TypeError} if wrong arguments.
 */
exports.getPipeConsumerRtpParameters = function(consumableParams)
{
	const consumerParams =
	{
		codecs           : [],
		headerExtensions : [],
		encodings        : [],
		rtcp             : consumableParams.rtcp
	};

	const consumableCodecs = utils.clone(consumableParams.codecs || []);

	for (const codec of consumableCodecs)
	{
		if (/.+\/rtx$/i.test(codec.mimeType))
			continue;

		// Reduce RTCP feedbacks by removing NACK support and other features.
		codec.rtcpFeedback = codec.rtcpFeedback.filter((fb) => (
			(fb.type === 'nack' && fb.parameter === 'pli') ||
			(fb.type === 'ccm' && fb.parameter === 'fir')
		));

		consumerParams.codecs.push(codec);
	}

	// Reduce RTP header extensions.
	consumerParams.headerExtensions = consumableParams.headerExtensions
		.filter((ext) => (
			ext.uri !== 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time'
		));

	const consumableEncodings = utils.clone(consumableParams.encodings || []);

	for (const encoding of consumableEncodings)
	{
		delete encoding.rtx;

		consumerParams.encodings.push(encoding);
	}

	return consumerParams;
};

function assertCodecCapability(codec)
{
	const valid =
		(typeof codec === 'object' && !Array.isArray(codec)) &&
		(typeof codec.mimeType === 'string' && codec.mimeType) &&
		(typeof codec.clockRate === 'number' && codec.clockRate);

	if (!valid)
		throw new TypeError('invalid RTCRtpCodecCapability');

	// Add kind if not present.
	if (!codec.kind)
		codec.kind = codec.mimeType.replace(/\/.*/, '').toLowerCase();
}

function assertCodecParameters(codec)
{
	const valid =
		(typeof codec === 'object' && !Array.isArray(codec)) &&
		(typeof codec.mimeType === 'string' && codec.mimeType) &&
		(typeof codec.clockRate === 'number' && codec.clockRate);

	if (!valid)
		throw new TypeError('invalid RTCRtpCodecParameters');
}

function matchCodecs(aCodec, bCodec)
{
	const aMimeType = aCodec.mimeType.toLowerCase();
	const bMimeType = bCodec.mimeType.toLowerCase();

	if (aMimeType !== bMimeType)
		return false;

	if (aCodec.clockRate !== bCodec.clockRate)
		return false;

	if (
		/^audio\/.+$/i.test(aMimeType) &&
		(
			(aCodec.channels !== undefined && aCodec.channels !== 1) ||
			(bCodec.channels !== undefined && bCodec.channels !== 1)
		) &&
		aCodec.channels !== bCodec.channels
	)
	{
		return false;
	}

	// Per codec special checks.
	switch (aMimeType)
	{
		case 'video/h264':
		{
			const aPacketizationMode = (aCodec.parameters || {})['packetization-mode'] || 0;
			const bPacketizationMode = (bCodec.parameters || {})['packetization-mode'] || 0;

			if (aPacketizationMode !== bPacketizationMode)
				return false;

			const aProfileLevelId = (aCodec.parameters || {})['profile-level-id'];
			const bProfileLevelId = (bCodec.parameters || {})['profile-level-id'];

			if (!aProfileLevelId || aProfileLevelId !== bProfileLevelId)
				return false;

			break;
		}
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
