'use strict';

const sdpTransform = require('sdp-transform');
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

		const transport = this._peer.transports[0];

		return Promise.resolve()
			// First create transceivers for receiving media (just if this is the
			// first SDP O/A).
			.then(() =>
			{
				if (!this._initialNegotiationDone)
				{
					const offerToReceiveAudio =
						this._initialCreateOfferOptions.offerToReceiveAudio;

					// For each N in offerToReceiveAudio create a transceiver ready to
					// receive audio.
					for (let i = 0; i < offerToReceiveAudio; i++)
					{
						const mid = `recv-audio-track-${i+1}`;
						const transceiver = new RTCRtpTransceiver(mid, 'audio');

						transceiver.capabilities = this._peer.capabilities;
						transceiver.transport = transport;
						transceiver.producer = null;

						// Store in the map.
						this._mapTransceivers.set(mid, transceiver);
					}

					const offerToReceiveVideo =
						this._initialCreateOfferOptions.offerToReceiveVideo;

					// For each N in offerToReceiveAudio create a transceiver ready to
					// receive video.
					for (let i = 0; i < offerToReceiveVideo; i++)
					{
						const mid = `recv-video-track-${i+1}`;
						const transceiver = new RTCRtpTransceiver(mid, 'video');

						transceiver.capabilities = this._peer.capabilities;
						transceiver.transport = transport;
						transceiver.producer = null;

						// Store in the map.
						this._mapTransceivers.set(mid, transceiver);
					}
				}
			})
			// Then handle existing Consumers.
			.then(() =>
			{
				const promises = [];

				// Inspect the already existing Consumers and handle them.
				for (const consumer of this._peer.consumers)
				{
					const mid = consumer.rtpParameters.muxId;

					// Ignore if already handled.
					if (this._mapTransceivers.has(mid))
						continue;

					// Set the transport.
					const promise = consumer.setTransport(transport)
						.then(() =>
						{
							consumer.on('close', () =>
							{
								this._logger.debug('consumer "close" event');

								// Set flag.
								this._negotiationNeeded = true;

								// Try to renegotiate.
								this._mayRenegotiate();
							});

							consumer.on('parameterschange', () =>
							{
								this._logger.debug('consumer "parameterschange" event');

								// Set flag.
								this._negotiationNeeded = true;

								// Try to renegotiate.
								this._mayRenegotiate();
							});

							consumer.on('activechange', () =>
							{
								this._logger.debug('consumer "activechange" event');

								// Set flag.
								this._negotiationNeeded = true;

								// Try to renegotiate.
								this._mayRenegotiate();
							});
						})
						// Create the transceiver.
						.then(() =>
						{
							const transceiver = new RTCRtpTransceiver(mid, consumer.kind);

							transceiver.capabilities = this._peer.capabilities;
							transceiver.transport = transport;
							transceiver.consumer = consumer;

							// Store in the map.
							this._mapTransceivers.set(mid, transceiver);
						})
						.catch((error) =>
						{
							// It may happen that, at this point, the Consumer was internally
							// closed, so don't abort everythin due to this.
							this._logger.error(`consumer.setTransport() failed: ${error}`);
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

				if (remoteDtlsParameters)
				{
					this._gotRemoteDtlsParameters = true;

					return transport.setRemoteDtlsParameters(remoteDtlsParameters);
				}
				else
				{
					this._logger.warn(
						'no remote DTLS parameters found in SDP initial answer');
				}
			})
			// Then create the corresponding Producers.
			.then(() =>
			{
				const promises = [];

				for (const parsedMedia of parsed.media)
				{
					if (parsedMedia.direction !== 'sendonly')
						continue;

					const promise = this._createProducer(parsedMedia)
						.then((producer) =>
						{
							// Assign it to the already existing transceiver.
							const mid = parsedMedia.mid;
							const transceiver = this._mapTransceivers.get(mid);

							transceiver.producer = producer;
						});

					promises.push(promise);
				}

				return Promise.all(promises);
			});
	}

	_handleRemoteReAnswer(remoteDescription)
	{
		this._logger.debug('_handleRemoteReAnswer()');

		const self = this;
		const parsed = remoteDescription.parsed;
		const previousTransceivers = Array.from(this._mapTransceivers.values());
		const promises = [];

		// If not transport was negotiated yet, do it now.
		if (!this._gotRemoteDtlsParameters)
		{
			const remoteDtlsParameters = sdpUtils.descToRemoteDtlsParameters(parsed);

			if (remoteDtlsParameters)
			{
				this._gotRemoteDtlsParameters = true;

				const transport = this._peer.transports[0];
				const promise = transport.setRemoteDtlsParameters(remoteDtlsParameters);

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
			const parsedMedia = parsed.media[i];
			const transceiver = previousTransceivers[i];
			const promise = updateTransceiver(transceiver, parsedMedia);

			promises.push(promise);
		}

		return Promise.all(promises);

		function updateTransceiver(transceiver, parsedMedia)
		{
			const mid = transceiver.mid;

			// MID must match.
			if (parsedMedia.mid && parsedMedia.mid !== mid)
			{
				return Promise.reject(
					new Error(
						'MID values do not match ' +
						`[previous mid:${mid}, new mid:${parsedMedia.mid}]`));
			}

			// It is a receiving transceiver.
			if (transceiver.direction === 'recvonly')
			{
				// Peer has added a sending track on a slot reserved for it.
				if (!transceiver.producer && parsedMedia.direction === 'sendonly')
				{
					self._logger.debug(
						'_handleRemoteReAnswer() | new receiving transceiver [mid:%s]', mid);

					return self._createProducer(parsedMedia)
						.then((producer) =>
						{
							// Assign it to the already existing transceiver.
							const transceiver2 = self._mapTransceivers.get(parsedMedia.mid);

							transceiver2.producer = producer;
						});
				}
				// Peer has closed a sending track.
				else if (
					transceiver.producer &&
					!transceiver.producer.closed &&
					(parsedMedia.direction === 'inactive' || parsedMedia.port === 0))
				{
					self._logger.debug(
						'_handleRemoteReAnswer() | receiving transceiver closed [mid:%s]',
						mid);

					transceiver.close();

					// Create a new transceiver and place in the same position.
					const newTransceiver =
						new RTCRtpTransceiver(transceiver.mid, transceiver.kind);

					newTransceiver.capabilities = self._peer.capabilities;
					newTransceiver.transport = transceiver.transport;
					newTransceiver.producer = null;

					self._mapTransceivers.set(newTransceiver.mid, newTransceiver);
				}
			}
		}
	}

	_createLocalDescription(type)
	{
		this._logger.debug('_createLocalDescription() [type:%s]', type);

		const obj = {};
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
				mids : '' // NOTE: This is filled below.
			}
		];
		obj.media = [];

		// DTLS fingerprint.
		obj.fingerprint =
		{
			type : 'sha-256',
			hash : transport.dtlsLocalParameters.fingerprints['sha-256']
		};

		// Iterate all the transceivers and add a m= section for each one.
		for (const transceiver of this._mapTransceivers.values())
		{
			const objMedia = transceiver.toObjectMedia(type);

			obj.media.push(objMedia);
		}

		// Add the pending session a=group attribute.
		obj.groups[0].mids = Array.from(this._mapTransceivers.values())
			.map((transceiver) => transceiver.mid)
			.join(' ');

		// Create local RTCSessionDescription.
		const localDescription = new RTCSessionDescription(
			{
				type : type,
				sdp  : obj
			});

		return localDescription;
	}

	_createProducer(parsedMedia)
	{
		// TODO: We assume no RTX/FEC in client's generated answer.

		const transport = this._peer.transports[0];
		const kind = parsedMedia.type;
		const mid = parsedMedia.mid;
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

		// Array of RtpEncodingParameters.
		// NOTE: Just a single encoding will be created with the first media codec
		// in the answer.
		const encodings = [];
		const encoding =
		{
			ssrc             : parsedMedia.ssrcs[0].id,
			codecPayloadType : mapCodecs.values().next().value.payloadType
		};

		if (hasRtx && parsedMedia.ssrcs.length >= 2)
			encoding.rtx = { ssrc: parsedMedia.ssrcs[1].id };

		encodings.push(encoding);

		// Array of RtpHeaderExtensionParameters.
		const headerExtensions =
			sdpUtils.descToRtpHeaderExtensionParameters(parsedMedia);

		// Create a Producer.
		const producer = this._peer.Producer(kind, transport);

		// RtpParameters object.
		const rtpParameters =
		{
			muxId            : mid,
			codecs           : Array.from(mapCodecs.values()),
			encodings        : encodings,
			headerExtensions : headerExtensions,
			rtcp             :
			{
				cname : parsedMedia.ssrcs[0].value
			},
			userParameters :
			{
				msid : parsedMedia.msid
			}
		};

		// Tell the Producer to receive and return the Promise.
		return producer.receive(rtpParameters);
	}
}

module.exports = RTCPeerConnectionUnifiedPlan;
