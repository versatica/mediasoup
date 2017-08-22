'use strict';

const sdpTransform = require('sdp-transform');
const utils = require('../../utils');
const RTCPeerConnectionCommon = require('./RTCPeerConnectionCommon');
const RTCSessionDescription = require('../RTCSessionDescription');
const sdpUtils = require('./sdpUtils');

const KIND_TO_MID =
{
	audio : 'audio-tracks',
	video : 'video-tracks'
};

class RTCPeerConnectionPlanB extends RTCPeerConnectionCommon
{
	constructor(options)
	{
		super(options);

		// Map of RtpReceivers indexed by MediaStreamTrack.id (a=label).
		this._mapTrackToRtpReceiver = new Map();
	}

	_setUpOffer()
	{
		this._logger.debug('_setUpOffer()');

		const transport = this._peer.transports[0];

		return Promise.resolve()
			// First handle all the RtpSenders.
			.then(() =>
			{
				const promises = [];

				// Inspect the already existing RtpSenders and handle them.
				for (const rtpSender of this._peer.rtpSenders)
				{
					// Ignore if it already has a transport (so it was already handled).
					// TODO: I don't like this. It's better to handle my own set of
					// handled RtpSenders.
					if (rtpSender.transport)
						continue;

					// Set the transport.
					const promise = rtpSender.setTransport(transport)
						.then(() =>
						{
							rtpSender.on('close', () =>
							{
								this._logger.debug('rtpSender "close" event');

								// Set flag.
								this._negotiationNeeded = true;

								// Try to renegotiate.
								this._mayRenegotiate();
							});

							rtpSender.on('parameterschange', () =>
							{
								this._logger.debug('rtpSender "parameterschange" event');

								// Set flag.
								this._negotiationNeeded = true;

								// Try to renegotiate.
								this._mayRenegotiate();
							});

							rtpSender.on('activechange', (active) =>
							{
								this._logger.debug('rtpSender "activechange" event [active:%s]', active);

								// Set flag.
								this._negotiationNeeded = true;

								// Try to renegotiate.
								this._mayRenegotiate();
							});
						})
						.catch((error) =>
						{
							// It may happen that, at this point, the RtpSender was internally
							// closed, so don't abort everythin due to this.
							this._logger.error(`rtpSender.setTransport() failed: ${error}`);
						});

					promises.push(promise);
				}

				return Promise.all(promises);
			});
	}

	_handleRemoteInitialAnswer(remoteDescription)
	{
		this._logger.debug('_handleRemoteInitialAnswer()');

		const parsed = remoteDescription.parsed;

		return Promise.resolve()
			// First sets the DTLS remote parameters into our local transport.
			.then(() =>
			{
				const transport = this._peer.transports[0];
				const remoteDtlsParameters = sdpUtils.descToRemoteDtlsParameters(parsed);

				if (!remoteDtlsParameters)
					throw new Error('no DTLS remote parameters found');

				return transport.setRemoteDtlsParameters(remoteDtlsParameters);
			})
			// Then create the corresponding RtpReceivers.
			.then(() =>
			{
				const promises = [];

				for (const parsedMedia of parsed.media)
				{
					// Ignore if a=inactive (no compatible codecs).
					if (parsedMedia.direction === 'inactive')
						continue;

					// Get all the media ssrcs.
					const ssrcs = sdpUtils.getPlanBSsrcs(parsedMedia);

					for (const ssrc of ssrcs)
					{
						// Get MediaStream and MediaStreamTracks info.
						const trackInfo = sdpUtils.getPlanBTrackInfo(parsedMedia, ssrc);
						const promise = this._createRtpReceiverForSsrc(parsedMedia, ssrc, trackInfo)
							.then((rtpReceiver) =>
							{
								// Store it in the map.
								this._mapTrackToRtpReceiver.set(trackInfo.trackId, rtpReceiver);

								// When closed, remove from the map.
								rtpReceiver.on('close', () =>
								{
									this._mapTrackToRtpReceiver.delete(trackInfo.trackId);
								});
							});

						promises.push(promise);
					}
				}

				return Promise.all(promises);
			});
	}

	_handleRemoteReAnswer(remoteDescription)
	{
		this._logger.debug('_handleRemoteReAnswer()');

		const parsed = remoteDescription.parsed;

		return Promise.resolve()
			// Check new/removed RtpReceivers.
			.then(() =>
			{
				const receivingTrackIds = new Set();
				const promises = [];

				for (const parsedMedia of parsed.media)
				{
					// Ignore if a=inactive (no compatible codecs).
					if (parsedMedia.direction === 'inactive')
						continue;

					// Get all the media ssrcs.
					const ssrcs = sdpUtils.getPlanBSsrcs(parsedMedia);

					for (const ssrc of ssrcs)
					{
						// Get MediaStream and MediaStreamTracks info.
						const trackInfo = sdpUtils.getPlanBTrackInfo(parsedMedia, ssrc);

						receivingTrackIds.add(trackInfo.trackId);

						if (!this._mapTrackToRtpReceiver.has(trackInfo.trackId))
						{
							this._logger.debug('_handleRemoteReAnswer() | new receiving track [ssrc:%s, trackId:"%s"]',
								ssrc, trackInfo.trackId);

							const promise = this._createRtpReceiverForSsrc(parsedMedia, ssrc, trackInfo)
								.then((rtpReceiver) =>
								{
									// Store it in the map.
									this._mapTrackToRtpReceiver.set(trackInfo.trackId, rtpReceiver);

									// When closed, remove from the map.
									rtpReceiver.on('close', () =>
									{
										this._mapTrackToRtpReceiver.delete(trackInfo.trackId);
									});
								});

							promises.push(promise);
						}
					}
				}

				// Check removed receiving SSRC.
				for (const trackId of this._mapTrackToRtpReceiver.keys())
				{
					if (!receivingTrackIds.has(trackId))
					{
						this._logger.debug('_handleRemoteReAnswer() | receiving track removed [trackId:"%s"]', trackId);

						const rtpReceiver = this._mapTrackToRtpReceiver.get(trackId);

						rtpReceiver.close();
					}
				}

				return Promise.all(promises);
			});
	}

	_createLocalDescription(type)
	{
		this._logger.debug('_createLocalDescription() [type:%s]', type);

		const self = this;
		const obj = {};
		const kinds = Object.keys(KIND_TO_MID);
		const capabilities = this._peer.capabilities;
		const transport = this._peer.transports[0];

		// Increase SDP version if an offer.
		if (type === 'offer')
			this._sdpGlobalFields.version++;

		obj.version = 0;
		obj.origin =
		{
			address        : '0.0.0.0',
			ipVer          : 4,
			netType        : 'IN',
			sessionId      : this._sdpGlobalFields.id,
			sessionVersion : this._sdpGlobalFields.version,
			username       : 'mediasoup'
		};
		obj.name = '-';
		obj.timing = { start: 0, stop: 0 };
		obj.icelite = 'ice-lite'; // Important.
		obj.msidSemantic =
		{
			semantic : 'WMS',
			token    : '*'
		};
		obj.groups =
		[
			{
				type : 'BUNDLE',
				mids : kinds.map((k) => KIND_TO_MID[k]).join(' ')
			}
		];
		obj.media = [];

		// DTLS fingerprint.
		obj.fingerprint =
		{
			type : 'sha-256',
			hash : transport.dtlsLocalParameters.fingerprints['sha-256']
		};

		// Create a SDP media section for each media kind.
		for (const kind of kinds)
		{
			addMediaSection(kind);
		}

		// Create local RTCSessionDescription.
		const localDescription = new RTCSessionDescription(
			{
				type : type,
				sdp  : obj
			});

		return localDescription;

		function addMediaSection(kind)
		{
			const objMedia = {};

			// m= line.
			objMedia.type = kind;
			objMedia.port = 7;
			objMedia.protocol = 'RTP/SAVPF';

			// c= line.
			objMedia.connection = { ip: '127.0.0.1', version: 4 };

			// a=mid attribute.
			objMedia.mid = KIND_TO_MID[kind];

			// ICE.
			objMedia.iceUfrag = transport.iceLocalParameters.usernameFragment;
			objMedia.icePwd = transport.iceLocalParameters.password;
			objMedia.candidates = [];

			for (const candidate of transport.iceLocalCandidates)
			{
				const objCandidate = {};

				// mediasoup does not support non rtcp-mux so candidates component is
				// always RTP (1).
				objCandidate.component = 1;
				objCandidate.foundation = candidate.foundation;
				objCandidate.ip = candidate.ip;
				objCandidate.port = candidate.port;
				objCandidate.priority = candidate.priority;
				objCandidate.transport = candidate.protocol;
				objCandidate.type = candidate.type;
				if (candidate.tcpType)
					objCandidate.tcptype = candidate.tcpType;

				objMedia.candidates.push(objCandidate);
			}

			objMedia.endOfCandidates = 'end-of-candidates';

			// Announce support for ICE renomination.
			//   https://tools.ietf.org/html/draft-thatcher-ice-renomination
			objMedia.iceOptions = 'renomination';

			// DTLS.
			// If 'offer' always use 'actpass'.
			if (type === 'offer')
				objMedia.setup = 'actpass';
			else
				objMedia.setup = transport.dtlsLocalParameters.role === 'client' ? 'active' : 'passive';

			// a=direction attribute.
			objMedia.direction = 'sendrecv';

			objMedia.rtp = [];
			objMedia.rtcpFb = [];
			objMedia.fmtp = [];

			// Array of payload types.
			const payloads = [];

			// Codecs.
			for (const codec of capabilities.codecs)
			{
				if (codec.kind && codec.kind !== kind)
					continue;

				payloads.push(codec.payloadType);

				const codecSubtype = codec.name.split('/')[1];
				const rtp =
				{
					payload : codec.payloadType,
					codec   : codecSubtype,
					rate    : codec.clockRate
				};

				if (codec.numChannels > 1)
					rtp.encoding = codec.numChannels;

				objMedia.rtp.push(rtp);

				// If codec has parameters add them into a=fmtp attributes.
				if (codec.parameters)
				{
					const paramFmtp =
					{
						payload : codec.payloadType,
						config  : ''
					};

					for (const key of Object.keys(codec.parameters))
					{
						const normalizedKey = utils.paramToSDP(key);

						if (paramFmtp.config)
							paramFmtp.config += ';';
						paramFmtp.config += `${normalizedKey}=${codec.parameters[key]}`;
					}

					if (paramFmtp.config)
						objMedia.fmtp.push(paramFmtp);
				}

				// Set RTCP feedback.
				if (codec.rtcpFeedback)
				{
					for (const fb of codec.rtcpFeedback)
					{
						objMedia.rtcpFb.push(
							{
								payload : codec.payloadType,
								type    : fb.type,
								subtype : fb.parameter
							});
					}
				}
			}

			// If there are no codecs, set this m section as unavailable.
			if (payloads.length === 0)
			{
				objMedia.payloads = '7'; // I don't care.
				objMedia.port = 0;
				objMedia.direction = 'inactive';
			}
			else
			{
				// Codec payloads.
				objMedia.payloads = payloads.join(' ');

				// SSRCs.
				objMedia.ssrcs = [];

				// SSRC groups.
				objMedia.ssrcGroups = [];

				for (const rtpSender of self._peer.rtpSenders)
				{
					// Just the same kind.
					if (rtpSender.kind !== kind)
						continue;

					// Just if active.
					if (!rtpSender.active)
						continue;

					for (const encoding of rtpSender.rtpParameters.encodings)
					{
						objMedia.ssrcs.push(
							{
								id        : encoding.ssrc,
								attribute : 'cname',
								value     : rtpSender.rtpParameters.rtcp.cname
							});

						objMedia.ssrcs.push(
							{
								id        : encoding.ssrc,
								attribute : 'msid',
								value     : rtpSender.rtpParameters.userParameters.msid
							});

						// RTX.
						if (encoding.rtx !== undefined && encoding.rtx.ssrc !== 0)
						{
							objMedia.ssrcs.push(
								{
									id        : encoding.rtx.ssrc,
									attribute : 'cname',
									value     : rtpSender.rtpParameters.rtcp.cname
								});

							objMedia.ssrcs.push(
								{
									id        : encoding.rtx.ssrc,
									attribute : 'msid',
									value     : rtpSender.rtpParameters.userParameters.msid
								});

							// Associate original and retransmission SSRC.
							objMedia.ssrcGroups.push(
								{
									semantics : 'FID',
									ssrcs     : [ encoding.ssrc, encoding.rtx.ssrc ].join(' ')
								});
						}
					}
				}

				// RTP header extensions.
				objMedia.ext = [];

				for (const headerExtension of capabilities.headerExtensions)
				{
					if (headerExtension.kind && headerExtension.kind !== kind)
						continue;

					objMedia.ext.push(
						{
							value : headerExtension.preferredId,
							uri   : headerExtension.uri
						});
				}

				// a=rtcp-mux attribute.
				objMedia.rtcpMux = 'rtcp-mux';

				// a=rtcp-rsize
				objMedia.rtcpRsize = 'rtcp-rsize';

				// a=x-google-flag:conference.
				// TODO: Just set this if enabled via API (and once simulcast is implemented).
				// if (kind === 'video')
				// 	objMedia.xGoogleFlag = 'conference';
			}

			// Add the media section.
			obj.media.push(objMedia);
		}
	}

	_createRtpReceiverForSsrc(parsedMedia, mediaSsrcs, trackInfo)
	{
		// TODO: We assume no FEC in client's SDP.
		let hasRtx = false;

		if (Array.isArray(parsedMedia.ssrcGroups))
		{
			for (const group of parsedMedia.ssrcGroups)
			{
				switch (group.semantics)
				{
					case 'FID':
					{
						hasRtx = true;
						break;
					}
				}
			}
		}

		if (!Array.isArray(mediaSsrcs))
			mediaSsrcs = [ mediaSsrcs ];

		const transport = this._peer.transports[0];
		const kind = parsedMedia.type;
		// Map of RtpCodecParameters indexed by payload type.
		const mapCodecs = new Map();

		for (const rtp of parsedMedia.rtp)
		{
			// TODO: This will fail for kind:depth and video/vp8.
			const codecMime = `${kind}/${rtp.codec}`;
			const codec =
			{
				name         : codecMime,
				payloadType  : rtp.payload,
				clockRate    : rtp.rate,
				rtcpFeedback : [],
				parameters   : {}
			};

			if (rtp.encoding > 1)
				codec.numChannels = rtp.encoding;

			mapCodecs.set(rtp.payload, codec);
		}

		if (parsedMedia.fmtp)
		{
			for (const fmtp of parsedMedia.fmtp)
			{
				const params = sdpTransform.parseFmtpConfig(fmtp.config);
				const codec = mapCodecs.get(fmtp.payload);

				if (!codec)
					continue;

				codec.parameters = params;
			}
		}

		if (parsedMedia.rtcpFb)
		{
			for (const fb of parsedMedia.rtcpFb)
			{
				const codec = mapCodecs.get(fb.payload);

				if (!codec)
					continue;

				codec.rtcpFeedback.push(
					{
						type      : fb.type,
						parameter : fb.subtype
					});
			}
		}

		// Array of RtpEncodingParameters.
		const encodings = [];
		let codecPayloadType;

		// NOTE: We assume that the first codec in the list is the media one.
		if (mapCodecs.size > 0)
			codecPayloadType = mapCodecs.values().next().value.payloadType;

		if (codecPayloadType)
		{
			const encoding =
			{
				ssrc             : mediaSsrcs[0],
				codecPayloadType : codecPayloadType
			};

			if (hasRtx && mediaSsrcs.length >= 2)
				encoding.rtx = { ssrc: mediaSsrcs[1] };

			encodings.push(encoding);
		}

		// Array of RtpHeaderExtensionParameters.
		const headerExtensions = sdpUtils.descToRtpHeaderExtensionParameters(parsedMedia);

		// Create a RtpReceiver.
		const rtpReceiver = this._peer.RtpReceiver(kind, transport);

		// RtpParameters object.
		const rtpParameters =
		{
			codecs           : Array.from(mapCodecs.values()),
			encodings        : encodings,
			headerExtensions : headerExtensions,
			rtcp             :
			{
				cname : trackInfo.cname
			},
			userParameters :
			{
				msid : trackInfo.msid
			}
		};

		// Tell the RtpReceiver to receive and return the Promise.
		return rtpReceiver.receive(rtpParameters);
	}
}

module.exports = RTCPeerConnectionPlanB;
