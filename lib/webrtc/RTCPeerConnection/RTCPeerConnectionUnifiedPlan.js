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
		// Whether we got remote DTLS parameters (it may not happen during the first
		// SDP O/A.
		this._gotRemoteDtlsParameters = false;
	}

	_setUpOffer()
	{
		this._logger.debug('_setUpOffer()');

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
						let transceiver = new RTCRtpTransceiver(mid, 'audio');

						transceiver.capabilities = this._peer.capabilities;
						transceiver.transport = transport;
						transceiver.receiver = null;

						// Store in the map.
						this._mapTransceivers.set(mid, transceiver);
					}

					// For each N in offerToReceiveAudio create a transceiver ready to receive
					// video.
					for (let i = 0; i < this._initialCreateOfferOptions.offerToReceiveVideo; i++)
					{
						let mid = `recv-video-track-${i+1}`;
						let transceiver = new RTCRtpTransceiver(mid, 'video');

						transceiver.capabilities = this._peer.capabilities;
						transceiver.transport = transport;
						transceiver.receiver = null;

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

							rtpSender.on('activechange', () =>
							{
								this._logger.debug('rtpSender "activechange" event');

								// Set flag.
								this._negotiationNeeded = true;

								// Try to renegotiate.
								this._mayRenegotiate();
							});
						})
						// Create the transceiver.
						.then(() =>
						{
							let transceiver = new RTCRtpTransceiver(mid, rtpSender.kind);

							transceiver.capabilities = this._peer.capabilities;
							transceiver.transport = transport;
							transceiver.sender = rtpSender;

							// Store in the map.
							this._mapTransceivers.set(mid, transceiver);
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

		let parsed = remoteDescription.parsed;

		return Promise.resolve()
			// First sets the DTLS remote parameters into our local transport.
			.then(() =>
			{
				let transport = this._peer.transports[0];
				let remoteDtlsParameters = sdpUtils.descToRemoteDtlsParameters(parsed);

				if (remoteDtlsParameters)
				{
					this._gotRemoteDtlsParameters = true;

					return transport.setRemoteDtlsParameters(remoteDtlsParameters);
				}
				else
				{
					this._logger.warn('no remote DTLS parameters found in SDP initial answer');
				}
			})
			// Then create the corresponding RtpReceivers.
			.then(() =>
			{
				let promises = [];

				for (let parsedMedia of parsed.media)
				{
					if (parsedMedia.direction !== 'sendonly')
						continue;

					let promise = this._createRtpReceiver(parsedMedia)
						.then((rtpReceiver) =>
						{
							// Assign it to the already existing transceiver.
							let mid = parsedMedia.mid;
							let transceiver = this._mapTransceivers.get(mid);

							transceiver.receiver = rtpReceiver;
						});

					promises.push(promise);
				}

				return Promise.all(promises);
			});
	}

	_handleRemoteReAnswer(remoteDescription)
	{
		this._logger.debug('_handleRemoteReAnswer()');

		let self = this;
		let parsed = remoteDescription.parsed;
		let previousTransceivers = Array.from(this._mapTransceivers.values());
		let promises = [];

		// If not transport was negotiated yet, do it now.
		if (!this._gotRemoteDtlsParameters)
		{
			let remoteDtlsParameters = sdpUtils.descToRemoteDtlsParameters(parsed);

			if (remoteDtlsParameters)
			{
				this._gotRemoteDtlsParameters = true;

				let transport = this._peer.transports[0];
				let promise = transport.setRemoteDtlsParameters(remoteDtlsParameters);

				promises.push(promise);
			}
			else
			{
				this._logger.warn('no remote DTLS parameters found in SDP re-answer');
			}
		}

		// Iterate all the received m= sections.
		for (let i=0, len=parsed.media.length; i<len; i++)
		{
			let parsedMedia = parsed.media[i];
			let transceiver = previousTransceivers[i];
			let promise = updateTransceiver(transceiver, parsedMedia);

			promises.push(promise);
		}

		return Promise.all(promises);

		function updateTransceiver(transceiver, parsedMedia)
		{
			let mid = transceiver.mid;

			// MID must match.
			if (parsedMedia.mid && parsedMedia.mid !== mid)
				return Promise.reject(new Error(`MID values do not match [previous mid:${mid}, new mid:${parsedMedia.mid}]`));

			// It is a receiving transceiver.
			if (transceiver.direction === 'recvonly')
			{
				// Peer has added a sending track on a slot reserved for it.
				if (!transceiver.receiver && parsedMedia.direction === 'sendonly')
				{
					self._logger.debug('_handleRemoteReAnswer() | new receiving transceiver [mid:%s]', mid);

					return self._createRtpReceiver(parsedMedia)
						.then((rtpReceiver) =>
						{
							// Assign it to the already existing transceiver.
							let mid = parsedMedia.mid;
							let transceiver = self._mapTransceivers.get(mid);

							transceiver.receiver = rtpReceiver;
						});
				}
				// Peer has closed a sending track.
				else if (
					transceiver.receiver &&
					!transceiver.receiver.closed &&
					(parsedMedia.direction === 'inactive' || parsedMedia.port === 0))
				{
					self._logger.debug('_handleRemoteReAnswer() | receiving transceiver closed [mid:%s]', mid);

					transceiver.close();

					// Create a new transceiver and place in the same position.
					let newTransceiver = new RTCRtpTransceiver(transceiver.mid, transceiver.kind);

					newTransceiver.capabilities = self._peer.capabilities;
					newTransceiver.transport = transceiver.transport;
					newTransceiver.receiver = null;

					self._mapTransceivers.set(newTransceiver.mid, newTransceiver);
				}
			}
		}
	}

	_createLocalDescription(type)
	{
		this._logger.debug('_createLocalDescription() [type:%s]', type);

		let obj = {};
		let transport = this._peer.transports[0];

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

		// Iterate all the transceivers and add a m= section for each one.
		for (let transceiver of this._mapTransceivers.values())
		{
			let objMedia = transceiver.toObjectMedia(type);

			obj.media.push(objMedia);
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

	_createRtpReceiver(parsedMedia)
	{
		// TODO: We assume no RTX/FEC in client's generated answer.

		let transport = this._peer.transports[0];
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

		let hasRtx = false;

		if (Array.isArray(parsedMedia.ssrcGroups))
		{
			for (let group of parsedMedia.ssrcGroups)
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

		// Array of RtpEncodingParameters.
		// NOTE: Just a single encoding will be created with the first media codec
		// in the answer.
		let encodings = [];
		let encoding =
		{
				ssrc             : parsedMedia.ssrcs[0].id,
				codecPayloadType : mapCodecs.values().next().value.payloadType
		};

		if (hasRtx && parsedMedia.ssrcs.length >= 2)
			encoding.rtx = { ssrc: parsedMedia.ssrcs[1].id };

		encodings.push(encoding);

		// Array of RtpHeaderExtensionParameters.
		let headerExtensions = sdpUtils.descToRtpHeaderExtensionParameters(parsedMedia);

		// Create a RtpReceiver.
		let rtpReceiver = this._peer.RtpReceiver(kind, transport);

		// RtpParameters object.
		let rtpParameters =
		{
			muxId            : mid,
			codecs           : Array.from(mapCodecs.values()),
			encodings        : encodings,
			headerExtensions : headerExtensions,
			rtcp             :
			{
				cname : parsedMedia.ssrcs[0].value
			},
			userParameters   :
			{
				msid : parsedMedia.msid
			}
		};

		// Tell the RtpReceiver to receive and return the Promise.
		return rtpReceiver.receive(rtpParameters);
	}
}

module.exports = RTCPeerConnectionUnifiedPlan;
