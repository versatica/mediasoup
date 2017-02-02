'use strict';

const sdpTransform = require('sdp-transform');
const extra = require('../../extra');
const RTCPeerConnectionCommon = require('./RTCPeerConnectionCommon');
const RTCSessionDescription = require('../RTCSessionDescription');
const RTCRtpTransceiver = require('./RTCRtpTransceiver');
const sdpUtils = require('./sdpUtils');

class RTCPeerConnectionUnifiedPlan extends RTCPeerConnectionCommon
{
	constructor(options)
	{
		super(options);

		// Ordered map of RTCRtpTransceivers indexed by MID.
		this._mapTransceivers = new Map();
	}

	_createInitialOffer()
	{
		this._logger.debug('_createInitialOffer()');

		let transport = this._peer.transports[0];

		return Promise.resolve()
			// First create transceivers for receiving media (just if this is the
			// first SDP O/A).
			.then(() =>
			{
				if (!this._initialNegotiationDone)
				{
					// For each N in offerToReceiveAudio create a transceiver ready to receive
					// audio.
					for (let i = 0; i < this._initialCreateOfferOptions.offerToReceiveAudio; i++)
					{
						let mid = `recv-audio-track-${i+1}`;
						let transceiver = new RTCRtpTransceiver(mid, 'audio', this._options);

						transceiver.capabilities = this._peer.capabilities;
						transceiver.transport = transport;
						transceiver.direction = 'recvonly';

						// Store in the map.
						this._mapTransceivers.set(mid, transceiver);
					}

					// For each N in offerToReceiveAudio create a transceiver ready to receive
					// video.
					for (let i = 0; i < this._initialCreateOfferOptions.offerToReceiveVideo; i++)
					{
						let mid = `recv-video-track-${i+1}`;
						let transceiver = new RTCRtpTransceiver(mid, 'video', this._options);

						transceiver.capabilities = this._peer.capabilities;
						transceiver.transport = transport;
						transceiver.direction = 'recvonly';

						// Store in the map.
						this._mapTransceivers.set(mid, transceiver);
					}
				}
			})
			// Then handle existing RtpSenders.
			.then(() =>
			{
				let promises = [];

				// Inspect the already existing RtpSenders and handle them.
				for (let rtpSender of this._peer.rtpSenders)
				{
					let mid = rtpSender.rtpParameters.muxId;

					// Ignore if already handled.
					if (this._mapTransceivers.has(mid))
						continue;

					// Set the transport.
					let promise = rtpSender.setTransport(transport)
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
						})
						// Create the transceiver.
						.then(() =>
						{
							let transceiver = new RTCRtpTransceiver(mid, rtpSender.kind, this._options);

							transceiver.capabilities = this._peer.capabilities;
							transceiver.transport = transport;
							transceiver.sender = rtpSender;

							// Store in the map.
							this._mapTransceivers.set(mid, transceiver);
						});

					promises.push(promise);
				}

				return Promise.all(promises);
			});
	}

	_handleRemoteInitialAnswer(remoteDescription)
	{
		this._logger.debug('_handleRemoteInitialAnswer()');

		let self = this;
		let parsed = remoteDescription.parsed;
		let transport = this._peer.transports[0];

		return Promise.resolve()
			// First sets the DTLS remote parameters into our local transport.
			.then(() =>
			{
				let remoteDtlsParameters = sdpUtils.descToRemoteDtlsParameters(parsed);

				return transport.setRemoteDtlsParameters(remoteDtlsParameters);
			})
			// Then create the corresponding RtpReceivers.
			.then(() =>
			{
				let promises = [];

				for (let parsedMedia of parsed.media)
				{
					if (parsedMedia.direction !== 'sendonly')
						continue;

					let promise = createRtpReceiver(parsedMedia);

					promises.push(promise);
				}

				return Promise.all(promises);
			});

		function createRtpReceiver(parsedMedia)
		{
			// TODO: We assume no RTX/FEC in client's generated answer.

			let kind = parsedMedia.type;
			let mid = parsedMedia.mid;
			// Map of RtpCodecParameters indexed by payload type.
			let mapCodecs = new Map();

			for (let rtp of parsedMedia.rtp)
			{
				// TODO: This will fail for kind:depth and video/vp8.
				let codecMime = kind + '/' + rtp.codec;
				let codec =
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
				for (let fmtp of parsedMedia.fmtp)
				{
					let params = sdpTransform.parseFmtpConfig(fmtp.config);
					let codec = mapCodecs.get(fmtp.payload);

					if (!codec)
						continue;

					for (let key of Object.keys(params))
					{
						let normalizedKey = extra.paramFromSDP(key);

						codec.parameters[normalizedKey] = params[key];
					}
				}
			}

			if (parsedMedia.rtcpFb)
			{
				for (let fb of parsedMedia.rtcpFb)
				{
					let codec = mapCodecs.get(fb.payload);

					if (!codec)
						continue;

					codec.rtcpFeedback.push(
						{
							type      : fb.type,
							parameter : fb.subtype
						});
				}
			}

			let mediaSsrc;
			let cname;
			let msid = parsedMedia.msid;

			// Take the first RTCP CNAME found.
			for (let ssrc of parsedMedia.ssrcs)
			{
				if (ssrc.attribute === 'cname')
				{
					mediaSsrc = ssrc.id;
					cname = ssrc.value;

					break;
				}
			}

			// Array of RtpEncodingParameters.
			// NOTE: Just a single encoding will be created with the first media codec
			// in the answer.
			let encodings =
			[
				{
					ssrc             : mediaSsrc,
					codecPayloadType : mapCodecs.values().next().value.payloadType
				}
			];

			// Array of RtpHeaderExtensionParameters.
			let headerExtensions = sdpUtils.descToRtpHeaderExtensionParameters(parsedMedia);

			// Create a RtpReceiver.
			let rtpReceiver = self._peer.RtpReceiver(kind, transport);

			// Assign it to the already existing transceiver.
			let transceiver = self._mapTransceivers.get(mid);

			transceiver.receiver = rtpReceiver;

			// RtpParameters object.
			let rtpParameters =
			{
				muxId            : mid,
				codecs           : Array.from(mapCodecs.values()),
				encodings        : encodings,
				headerExtensions : headerExtensions,
				rtcp             :
				{
					cname : cname
				},
				userParameters   :
				{
					msid : msid
				}
			};

			// TODO: REMOVE
			// self._logger.warn('------ calling rtpReceiver.receive() with\n%s', JSON.stringify(rtpParameters, null, '\t'));

			// Tell the RtpReceiver to receive and return the Promise.
			return rtpReceiver.receive(rtpParameters);
		}
	}

	_handleRemoteReOffer(remoteDescription) // eslint-disable-line no-unused-vars
	{
		this._logger.debug('_handleRemoteReOffer()');

		// TODO: Do it.

		return Promise.reject(new Error('renegotiation initiated by the endpoint not yet supported'));
	}

	_handleRemoteReAnswer(remoteDescription) // eslint-disable-line no-unused-vars
	{
		this._logger.debug('_handleRemoteReAnswer()');

		// TODO: Inspect changes in the SDP (or may be not).

		return Promise.resolve();
	}

	_createLocalDescription(type)
	{
		this._logger.debug('_createLocalDescription() [type:%s]', type);

		let obj = {};
		let sdpVersion = this._sdpGlobalFields.version;
		let transport = this._peer.transports[0];

		// Increase SDP version if an offer.
		if (type === 'offer')
			sdpVersion++;

		obj.version = 0;
		obj.origin =
		{
			address        : '0.0.0.0',
			ipVer          : 4,
			netType        : 'IN',
			sessionId      : this._sdpGlobalFields.id,
			sessionVersion : sdpVersion,
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
				mids : '' // NOTE: This is filled below.
			}
		];
		obj.media = [];

		// DTLS fingerprint.
		obj.fingerprint =
		{
			type : 'sha-256',
			hash : extra.fingerprintToSDP(transport.dtlsLocalParameters.fingerprints['sha-256'])
		};

		let isFirstBundled = true;

		// Iterate all the transceivers and add a m= section for each one.
		for (let transceiver of this._mapTransceivers.values())
		{
			let objMedia = transceiver.toObjectMedia(type, isFirstBundled);

			obj.media.push(objMedia);

			if (!transceiver.closed)
				isFirstBundled = false;
		}

		// Add the pending session a=group attribute.
		obj.groups[0].mids = Array.from(this._mapTransceivers.values())
			.map((transceiver) => transceiver.mid)
			.join(' ');

		// Create local RTCSessionDescription.
		let localDescription = new RTCSessionDescription(
			{
				type : type,
				sdp  : obj
			});

		return localDescription;
	}
}

module.exports = RTCPeerConnectionUnifiedPlan;
