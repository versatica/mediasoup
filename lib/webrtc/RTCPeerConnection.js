'use strict';

const EventEmitter = require('events').EventEmitter;
const sdpTransform = require('sdp-transform');
const randomString = require('random-string');
const logger = require('../logger');
const extra = require('../extra');
const InvalidStateError = require('../errors').InvalidStateError;

const SIGNALING_STATE =
{
	stable          : 'stable',
	haveLocalOffer  : 'have-local-offer',
	haveRemoteOffer : 'have-remote-offer'
};
const NEGOTIATION_NEEDED_DELAY = 1000;

let fakePort = 10000;

class RTCPeerConnection extends EventEmitter
{
	/**
	 * RTCPeerConnection constructor.
	 * @param  {Room} room
	 * @param  {string} peerName
	 * @param  {object} [options]
	 * @param  {object} [options.transportOptions] - Options for Transport.
	 * @param  {boolean} [options.usePlanB] - Use Plan-B rather than Unified-Plan.
	 * @param  {object} [options.bandwidth]
	 * @param  {number} [options.bandwidth.audio] - Maximum bandwidth for audio streams.
	 * @param  {number} [options.bandwidth.video] - Maximum bandwidth for video streams.
	 */
	constructor(room, peerName, options)
	{
		super();
		this.setMaxListeners(Infinity);

		this._logger = logger(`webrtc:RTCPeerConnection:${peerName}`);

		this._logger.debug('constructor() [options:%o]', options);

		// Room instance.
		this._room = room;

		// Peer name.
		this._peerName = peerName;

		// Options.
		this._options = options || {};

		// Peer instance.
		this._peer = null;

		// Peer effective capabilities.
		this._capabilities = null;

		// Local RTCSessionDescription.
		this._localDescription = null;

		// Remote RTCSessionDescription.
		this._remoteDescription = null;

		// Initial signaling state.
		this._signalingState = SIGNALING_STATE.stable;

		// Busy flag.
		this._busy = false;

		// Negotiation needed flag.
		this._negotiationNeeded = false;

		// Negotiation timer to collect them.
		this._negotiationNeededTimer = null;

		// SDP global fields.
		this._sdpFields =
		{
			id      : randomString({ letters: false, length: 16 }),
			version : 1
		};

		// Map of m= sections:
		// - key: mid
		// - value: RtpReceiver or RtpSender
		this._rtpHandlers = new Map();
	}

	get closed()
	{
		return this._peer ? this._peer.closed : false;
	}

	get peer()
	{
		return this._peer;
	}

	get localDescription()
	{
		return this._localDescription;
	}

	get remoteDescription()
	{
		return this._remoteDescription;
	}

	get signalingState()
	{
		return this._signalingState;
	}

	close()
	{
		this._logger.debug('close()');

		// Cancel the negotiation timer.
		clearTimeout(this._negotiationNeededTimer);

		// TODO: handle if Peer not yet created

		if (this._peer)
			this._peer.close();
	}

	createOffer()
	{
		this._logger.debug('createOffer()');

		let self = this;

		if (this._busy)
			return Promise.reject(new InvalidStateError('busy'));

		if (this._signalingState !== SIGNALING_STATE.stable)
		{
			return Promise.reject(new InvalidStateError(`invalid signaling state [signalingState:${this._signalingState}]`));
		}

		// Set busy flag.
		this._busy = true;

		// Refresh our RtpSenders.
		return handleRtpSenders()
			.then(() =>
			{
				this._logger.debug('createOffer() | succeed');

				// Increase our SDP version.
				this._sdpFields.version++;

				// Unset busy flag.
				this._busy = false;

				return this._createLocalDescription('offer');
			})
			.catch((error) =>
			{
				this._logger.error('createOffer() | failed: %s', error);

				// Unset busy flag.
				this._busy = false;

				throw error;
			});

		function handleRtpSenders()
		{
			let transport = self._peer.transports[0];
			let promises = [];

			// Inspect the already existing RtpSenders and handle them.
			for (let rtpSender of self._peer.rtpSenders)
			{
				let promise = self._handleRtpSender(rtpSender, transport);

				promises.push(promise);
			}

			return Promise.all(promises);
		}
	}

	createAnswer()
	{
		this._logger.debug('createAnswer()');

		if (this._signalingState !== SIGNALING_STATE.haveRemoteOffer)
		{
			return Promise.reject(new InvalidStateError(`invalid signaling state [signalingState:${this._signalingState}]`));
		}

		// Create an answer and resolve.
		return Promise.resolve(this._createLocalDescription('answer'));
	}

	setRemoteDescription(desc)
	{
		this._logger.debug('setRemoteDescription()');

		let self = this;
		let newSignalingState;

		if (this._busy)
			return Promise.reject(new InvalidStateError('busy'));

		switch (desc.type)
		{
			case 'offer':
			{
				if (this._signalingState !== SIGNALING_STATE.stable)
					return Promise.reject(new InvalidStateError(`invalid RTCSessionDescription.type [type:${desc.type}, signalingState:${this._signalingState}]`));

				// Set signaling state.
				newSignalingState = SIGNALING_STATE.haveRemoteOffer;

				break;
			}

			case 'answer':
			{
				if (this._signalingState !== SIGNALING_STATE.haveLocalOffer)
					return Promise.reject(new InvalidStateError(`invalid RTCSessionDescription.type [type:${desc.type}, signalingState:${this._signalingState}]`));

				// Set signaling state.
				newSignalingState = SIGNALING_STATE.stable;

				break;
			}

			default:
			{
				return Promise.reject(new Error(`invalid RTCSessionDescription.type [type:${desc.type}]`));
			}
		}

		let sdpObject;

		try
		{
			sdpObject = sdpTransform.parse(desc.sdp);
		}
		catch (error)
		{
			return Promise.reject(new Error(`invalid sdp: ${error}`));
		}

		// TODO: validate SDP (must have 1 or more medias, fingerprint, etc)

		// Offer.
		if (desc.type === 'offer')
		{
			if (this._peer)
				return Promise.reject(new Error('renegotiation not yet implemented'));

			// Update our SDP fields.
			this._sdpFields.version = sdpObject.origin.sessionVersion;

			// Set busy flag.
			this._busy = true;

			return this._createPeer(sdpObject)
				.then(() =>
				{
					return createTransport();
				})
				.then(() =>
				{
					return createRtpReceivers();
				})
				.then(() =>
				{
					this._logger.debug('setRemoteDescription() | succeed');

					// Unset busy flag.
					this._busy = false;

					// Set remote description.
					this._remoteDescription = desc;

					// Update signaling state.
					this._setSignalingState(newSignalingState);
				})
				.catch((error) =>
				{
					this._logger.error('setRemoteDescription() | failed: %s', error);

					// Unset busy flag.
					this._busy = false;

					throw error;
				});
		}
		// Answer.
		else
		{
			// Set remote description.
			this._remoteDescription = desc;

			// Update signaling state.
			this._setSignalingState(newSignalingState);

			return Promise.resolve();
		}

		function createTransport()
		{
			return self._peer.createTransport(self._options.transportOptions)
				.then((transport) =>
				{
					let media = sdpObject.media[0];
					// NOTE: Firefox uses global fingerprint attribute.
					let remoteFingerprint = media.fingerprint || sdpObject.fingerprint;

					// Set DSLT parameters into the transport.
					return transport.setRemoteDtlsParameters(
						{
							// SDP offer MUST always have a=setup:actpass so we can choose
							// whatever we want.
							role        : 'server',
							fingerprint :
							{
								algorithm : remoteFingerprint.type,
								value     : extra.fingerprintFromSDP(remoteFingerprint.hash)
							}
						});
				});
		}

		function createRtpReceivers()
		{
			let transport = self._peer.transports[0];
			let promises = [];

			// Create a RtpReceiver for each media.
			for (let media of sdpObject.media)
			{
				let promise = createRtpReceiver(media, transport);

				promises.push(promise);
			}

			return Promise.all(promises);
		}

		function createRtpReceiver(media, transport)
		{
			// TODO: validate there is a=mid, etc

			switch (media.type)
			{
				case 'audio':
				case 'video':
				{
					// TODO: this would fail if a=recvonly or there is no media.ssrcs

					// Media kind.
					let kind = media.type;
					// Array of SSRCs.
					let ssrcs = [];

					if (media.ssrcGroups)
					{
						ssrcs = media.ssrcGroups[0].ssrcs
							.split(' ')
							.map((ssrc) => Number(ssrc));
					}
					else
					{
						for (let ssrc of media.ssrcs)
						{
							ssrcs.push(ssrc.id);
						}
					}

					// SSRC for media.
					let mediaSsrc = ssrcs[0];
					// SSRC for RTX.
					let featureSsrc = ssrcs[1] || mediaSsrc;

					// Map of RtpCodecParameters indexed by payload type.
					let mapCodecs = new Map();
					// Set of RTX payloads.
					let setRtxPayloadTypes = new Set();

					for (let rtp of media.rtp)
					{
						// If it's a RTX codec add to the set of RTX payloads and continue.
						if (rtp.codec.toLowerCase() === 'rtx')
						{
							setRtxPayloadTypes.add(rtp.payload);
							continue;
						}

						// TODO: This will fail for kind:depth and video/vp8
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

					if (media.fmtp)
					{
						for (let fmtp of media.fmtp)
						{
							let params = sdpTransform.parseFmtpConfig(fmtp.config);

							// Check RTX.
							if (setRtxPayloadTypes.has(fmtp.payload))
							{
								// Must have 'apt'.
								if (!params.hasOwnProperty('apt'))
									throw new Error('rtx codec has no apt fmtp');

								// Set RTX parameters in the corresponding codec.
								let codec = mapCodecs.get(params.apt);

								if (!codec)
									continue;

								codec.rtx =
								{
									payloadType : fmtp.payload
								};

								if (params.hasOwnProperty('rtx-time'))
									codec.rtx.rtxTime = params['rtx-time'];
							}
							// Otherwise add codec parameters.
							else
							{
								let codec = mapCodecs.get(fmtp.payload);

								if (!codec)
									continue;

								for (let key in params)
								{
									if (params.hasOwnProperty(key))
									{
										let normalizedKey = extra.paramFromSDP(key);

										codec.parameters[normalizedKey] = params[key];
									}
								}
							}
						}
					}

					// Remove unsupported room codecs.
					for (let codec of mapCodecs.values())
					{
						// Must check whether the codec is present in the room capabilities.
						let roomCapabilities = self._room.getCapabilities(kind);
						let match = false;

						for (let codecCapability of roomCapabilities.codecs)
						{
							if (codec.name !== codecCapability.name)
								continue;

							if (codec.clockRate !== codecCapability.clockRate)
								continue;

							switch (codec.name.toLowerCase())
							{
								case 'video/h264':
								{
									if (!codec.parameters.packetizationMode)
										codec.parameters.packetizationMode = 0;

									if (codec.parameters.packetizationMode !== codecCapability.parameters.packetizationMode)
										continue;

									break;
								}
							}

							match = true;
							break;
						}

						if (!match)
							mapCodecs.delete(codec.payloadType);
					}

					if (media.rtcpFb)
					{
						for (let fb of media.rtcpFb)
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

					// Array of RtpEncodingParameters.
					let encodings = [];

					for (let codec of mapCodecs.values())
					{
						let codecSubtype = codec.name.split('/')[1].toLowerCase();

						// Ignore if it's not a media codec but a feature codec.
						switch (codecSubtype)
						{
							case 'rtx':
							case 'ulpfec':
							case 'flexfec':
							case 'red':
							case 'cn':
							case 'telephone-event':
								continue;
						}

						let encoding =
						{
							ssrc             : mediaSsrc,
							codecPayloadType : codec.payloadType
						};

						// If there is RTX add rtx object.
						if (setRtxPayloadTypes.size > 0)
							encoding.rtx = { ssrc: featureSsrc };

						encodings.push(encoding);
					}

					let rtcp = {};
					let msid;

					// Firefox places a=msid attribute.
					msid = media.msid;

					// Take the first RTCP CNAME found as it must be unique per peer.
					for (let ssrc of media.ssrcs)
					{
						switch (ssrc.attribute)
						{
							case 'cname':
								rtcp.cname = ssrc.value;
								break;

							// Chrome uses a=ssrc:xxxx msid:yyyy.
							case 'msid':
								msid = ssrc.value;
								break;
						}
					}

					// If there are no encodings it means that there are not compatible
					// media codecs, so create a fake RtpReceiver to make the SDP m=
					// lines happy.
					if (encodings.length === 0)
					{
						// Create a fake RtpReceiver.
						let fakeRtpReceiver =
						{
							closed        : true,
							kind          : kind,
							transport     : transport,
							rtpParameters :
							{
								muxId     : media.mid,
								codecs    : [],
								encodings : [],
								rtcp :
								{
									cname : rtcp.cname
								}
							},
							isRtpSender   : function() { return false; },
							isRtpReceiver : function() { return true; },
						};

						// Store in the map.
						self._rtpHandlers.set(media.mid, fakeRtpReceiver);

						return Promise.resolve();
					}

					// Create a RtpReceiver.
					let rtpReceiver = self._peer.RtpReceiver(kind, transport);

					// Store in the map.
					self._rtpHandlers.set(media.mid, rtpReceiver);

					// RtpParameters object.
					let rtpParameters =
					{
						muxId          : media.mid,
						codecs         : Array.from(mapCodecs.values()),
						encodings      : encodings,
						rtcp           : rtcp,
						userParameters : {}
					};

					// Add 'msid' as custom parameter.
					if (msid)
						rtpParameters.userParameters.msid = msid;

					// TODO: REMOVE
					// self._logger.warn('---------- calling rtpReceiver.receive() with parameters:\n%s',
					// 	JSON.stringify(rtpParameters, null, '\t'));

					// Tell the RtpReceiver to receive RTP and return the Promise.
					return rtpReceiver.receive(rtpParameters);
				}

				// TODO: DataChannels
				// TODO: other types?
			}
		}
	}

	setLocalDescription(desc)
	{
		this._logger.debug('setLocalDescription()');

		let newSignalingState;

		switch (desc.type)
		{
			case 'offer':
			{
				if (this._signalingState !== SIGNALING_STATE.stable)
					return Promise.reject(new InvalidStateError(`invalid RTCSessionDescription.type [type:${desc.type}, signalingState:${this._signalingState}]`));

				// Set signaling state.
				newSignalingState = SIGNALING_STATE.haveLocalOffer;

				break;
			}

			case 'answer':
			{
				if (this._signalingState !== SIGNALING_STATE.haveRemoteOffer)
					return Promise.reject(new InvalidStateError(`invalid RTCSessionDescription.type [type:${desc.type}, signalingState:${this._signalingState}]`));

				// Update signaling state.
				newSignalingState = SIGNALING_STATE.stable;

				break;
			}

			default:
			{
				return Promise.reject(new Error(`invalid RTCSessionDescription.type [type:${desc.type}]`));
			}
		}

		// Update the local description.
		this._localDescription = desc;

		// Update signaling state.
		this._setSignalingState(newSignalingState);

		// Resolve.
		return Promise.resolve();
	}

	_createPeer(sdpObject)
	{
		this._logger.debug('_createPeer()');

		this._peer = this._room.Peer(this._peerName);

		// Manage events.

		this._peer.on('close', (error) =>
		{
			this.emit('close', error);
		});

		this._peer.on('newrtpsender', () =>
		{
			this._logger.debug('peer "newrtpsender" event');

			// Set flag.
			this._negotiationNeeded = true;

			// Try to renegotiate.
			this._checkNegotiationNeeded();
		});

		// Map of RtpCodecParameters indexed by payload type.
		let mapCodecs = new Map();

		for (let media of sdpObject.media)
		{
			// Media kind.
			let kind = media.type;

			for (let rtp of media.rtp)
			{
				// Ignore RTX codecs.
				if (rtp.codec.toLowerCase() === 'rtx')
					continue;

				// TODO: This will fail for kind:depth and video/vp8
				let codecMime = kind + '/' + rtp.codec;
				let codec =
				{
					kind         : kind,
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

			if (media.fmtp)
			{
				for (let fmtp of media.fmtp)
				{
					let params = sdpTransform.parseFmtpConfig(fmtp.config);
					let codec = mapCodecs.get(fmtp.payload);

					if (!codec)
						continue;

					for (let key in params)
					{
						if (params.hasOwnProperty(key))
						{
							let normalizedKey = extra.paramFromSDP(key);

							codec.parameters[normalizedKey] = params[key];
						}
					}
				}
			}

			if (media.rtcpFb)
			{
				for (let fb of media.rtcpFb)
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
		}

		// Set peer capabilities.

		let capabilities =
		{
			codecs           : Array.from(mapCodecs.values()),
			headerExtensions : [],  // TODO
			fecMechanisms    : []   // TODO
		};

		// TODO: REMOVE
		// this._logger.warn('---------- calling peer.setCapabilities() with\n%s',
		// 	JSON.stringify(capabilities, null, '\t'));

		return this._peer.setCapabilities(capabilities)
			.then((capabilities) =>
			{
				this._capabilities = capabilities;

				// TODO: REMOVE
				// console.log('---- PEER EFFECTIVE CAPABILITIES:\n' + JSON.stringify(capabilities, null, '\t'));
			});
	}

	_handleRtpSender(rtpSender, transport)
	{
		this._logger.debug('_handleRtpSender()');

		let id = rtpSender.rtpParameters.muxId;

		// If already handled, ignore it.
		if (this._rtpHandlers.has(id))
			return Promise.resolve();

		return rtpSender.setTransport(transport)
			.then(() =>
			{
				// Store in the map.
				this._rtpHandlers.set(id, rtpSender);

				// Set events.

				rtpSender.on('close', () =>
				{
					this._logger.debug('rtpSender "close" event');

					// Set flag.
					this._negotiationNeeded = true;

					// Try to renegotiate.
					this._checkNegotiationNeeded();
				});

				rtpSender.on('parameterschange', () =>
				{
					this._logger.debug('rtpSender "parameterschange" event');

					// Set flag.
					this._negotiationNeeded = true;

					// Try to renegotiate.
					this._checkNegotiationNeeded();
				});
			});
	}

	_setSignalingState(signalingState)
	{
		this._logger.debug('_setSignalingState() [signalingState:%s]', signalingState);

		if (this._signalingState === signalingState)
			return;

		this._signalingState = signalingState;

		// Emit 'signalingstatechange'.
		this.emit('signalingstatechange');

		if (this._signalingState === SIGNALING_STATE.stable)
		{
			process.nextTick(() =>
			{
				this._checkNegotiationNeeded();
			});
		}
	}

	_checkNegotiationNeeded()
	{
		// If already scheduled, ignore.
		if (this._negotiationNeededTimer)
			return;

		// If busy, ignore.
		if (this._busy)
			return;

		// If signalingState is 'stable' we must check whether:
		// - negotiation-needed flag was enabled, or
		// - there are pending rtpSenders to handle
		// If so, emit 'negotiationneeded'.

		if (this._signalingState !== SIGNALING_STATE.stable)
			return;

		// Schedule the task.
		this._negotiationNeededTimer = setTimeout(() =>
		{
			this._negotiationNeededTimer = null;

			// Ignore if closed.
			if (this.closed)
				return;

			// If busy, ignore.
			if (this._busy)
				return;

			if (this._negotiationNeeded)
			{
				// Reset flag.
				this._negotiationNeeded = false;

				// Emit event.
				this.emit('negotiationneeded');
			}
		}, NEGOTIATION_NEEDED_DELAY);
	}

	_createLocalDescription(type)
	{
		if (!this._options.usePlanB)
			return this._createLocalDescriptionUnifiedPlan(type);
		else
			return this._createLocalDescriptionPlanB(type);
	}

	_createLocalDescriptionUnifiedPlan(type)
	{
		this._logger.debug('_createLocalDescriptionUnifiedPlan() [type:%s]', type);

		let obj = {};

		obj.version = 0;
		obj.origin =
		{
			address        : '0.0.0.0',
			ipVer          : 4,
			netType        : 'IN',
			sessionId      : this._sdpFields.id,
			sessionVersion : this._sdpFields.version,
			username       : 'mediasoup-webrtc'
		};
		obj.name = '-';
		obj.timing = { start: 0, stop: 0 };
		obj.icelite = 'ice-lite';
		obj.msidSemantic =
		{
			semantic : 'WMS',
			token    : '*'
		};
		obj.groups =
		[
			{
				type : 'BUNDLE',
				mids : Array.from(this._rtpHandlers.keys())
					.filter((mid) =>
					{
						let rtpHandler = this._rtpHandlers.get(mid);

						return !rtpHandler.closed;
					})
					.join(' ')
			}
		];
		obj.media = [];

		let transport = this._peer.transports[0];

		// DTLS fingerprint.
		obj.fingerprint =
		{
			type : 'sha-256',
			hash : extra.fingerprintToSDP(transport.dtlsLocalParameters.fingerprints['sha-256'])
		};

		let bundleTranspportSet = false;

		for (let rtpHandler of this._rtpHandlers.values())
		{
			let objMedia = {};
			let transport = rtpHandler.transport;
			let isClosedRtpSender = false;

			if (rtpHandler.isRtpSender() &&
				(rtpHandler.closed || !rtpHandler.available))
			{
				isClosedRtpSender = true;
			}

			// m= line.
			objMedia.type = rtpHandler.kind;
			objMedia.port = fakePort++;  // Fuck you SDP
			if (fakePort > 60000)
				fakePort = 10000;
			objMedia.protocol = 'RTP/SAVPF';

			// If closed or not available sender, set port 0.
			if (rtpHandler.closed || isClosedRtpSender)
				objMedia.port = 0;

			// c= line.
			objMedia.connection = { ip: '0.0.0.0', version: 4 };  // Fuck you SDP

			// mid.
			objMedia.mid = rtpHandler.rtpParameters.muxId;

			// ICE.
			objMedia.iceUfrag = transport.iceLocalParameters.usernameFragment;
			objMedia.icePwd = transport.iceLocalParameters.password;

			if (!bundleTranspportSet)
			{
				objMedia.candidates = [];

				for (let candidate of transport.iceLocalCandidates)
				{
					let objCandidate = {};

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
						objCandidate.tcpType = candidate.tcpType;

					objMedia.candidates.push(objCandidate);
				}
			}

			if (!rtpHandler.closed)
				bundleTranspportSet = true;

			// DTLS.

			// If 'offer' always use 'actpass'.
			if (type === 'offer')
				objMedia.setup = 'actpass';
			else
				objMedia.setup = transport.dtlsLocalParameters.role === 'client' ? 'active' : 'passive';

			// Media.

			if (rtpHandler.isRtpReceiver())
			{
				objMedia.direction = 'recvonly';
			}
			else
			{
				if (!isClosedRtpSender)
					objMedia.direction = 'sendonly';
				else
					objMedia.direction = 'inactive';
			}

			// SSRC for media.
			let mediaSsrc;
			// SSRC for RTX.
			let featureSsrc;
			// Array of payload types.
			let payloads = [];

			objMedia.payloads = '';
			objMedia.rtp = [];
			objMedia.rtcpFb = [];
			objMedia.fmtp = [];

			if (rtpHandler.isRtpReceiver() || !isClosedRtpSender)
			{
				for (let codec of rtpHandler.rtpParameters.codecs)
				{
					payloads.push(codec.payloadType);

					let codecSubtype = codec.name.split('/')[1];
					let rtp =
					{
						payload : codec.payloadType,
						codec   : codecSubtype,
						rate    : codec.clockRate
					};

					if (codec.numChannels > 1)
						rtp.encoding = codec.numChannels;

					objMedia.rtp.push(rtp);

					// If the codec has RTX add another SDP codec and a corresponding
					// fmtp entry.
					if (codec.rtx)
					{
						payloads.push(codec.rtx.payloadType);

						objMedia.rtp.push(
							{
								payload : codec.rtx.payloadType,
								codec   : 'rtx',
								rate    : codec.clockRate
							});

						let rtxFmtp =
						{
							payload : codec.rtx.payloadType,
							config  : `apt=${codec.payloadType}`
						};

						if (codec.rtx.rtxTime)
							rtxFmtp.config += `;rtx-time=${codec.rtx.rtxTime}`;

						objMedia.fmtp.push(rtxFmtp);
					}

					// If codec has parameters add them into fmtp.
					if (codec.parameters)
					{
						let paramFmtp =
						{
							payload : codec.payloadType,
							config  : ''
						};

						for (let key in codec.parameters)
						{
							if (codec.parameters.hasOwnProperty(key))
							{
								let normalizedKey = extra.paramToSDP(key);

								if (paramFmtp.config)
									paramFmtp.config += ';';
								paramFmtp.config += `${normalizedKey}=${codec.parameters[key]}`;
							}
						}

						if (paramFmtp.config)
							objMedia.fmtp.push(paramFmtp);
					}

					if (codec.rtcpFeedback)
					{
						for (let fb of codec.rtcpFeedback)
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

				objMedia.payloads = payloads.join(' ');
				objMedia.ssrcs = [];

				for (let encoding of rtpHandler.rtpParameters.encodings)
				{
					// TODO: Wrong when simulcast
					mediaSsrc = mediaSsrc || encoding.ssrc;

					// TODO: Same
					// TODO: Uncomment when implemented
					// if (encoding.rtx)
					// 	featureSsrc = featureSsrc || encoding.rtx.ssrc;
				}

				// SSRC (just for available RtpSender).
				if (rtpHandler.isRtpSender() && !isClosedRtpSender)
				{
					if (mediaSsrc)
					{
						objMedia.ssrcs.push(
							{
								id        : mediaSsrc,
								attribute : 'cname',
								value     : rtpHandler.rtpParameters.rtcp.cname
							});

						if (rtpHandler.rtpParameters.userParameters.msid)
							objMedia.msid = rtpHandler.rtpParameters.userParameters.msid;
					}

					if (featureSsrc)
					{
						objMedia.ssrcs.push(
							{
								id        : featureSsrc,
								attribute : 'cname',
								value     : rtpHandler.rtpParameters.rtcp.cname
							});
					}

					if (mediaSsrc && featureSsrc)
					{
						objMedia.ssrcGroups =
						[
							{
								semantics : 'FID',  // TODO: what is this?
								ssrcs     : `${mediaSsrc} ${featureSsrc}`
							}
						];
					}
				}
			}

			// RTCP.

			// Set rtcp-mux.
			objMedia.rtcpMux = 'rtcp-mux';
			// Set rtcp-mux-only.
			// TODO: https://github.com/clux/sdp-transform/issues/48
			// objMedia.rtcpMuxOnly = 'rtcp-mux-only';


			// Bandwidth.
			switch (rtpHandler.kind)
			{
				case 'audio':
				{
					if (this._options.bandwidth && this._options.bandwidth.audio)
					{
						objMedia.bandwidth =
						[
							{ type: 'AS', limit : this._options.bandwidth.audio }
						];
					}
					break;
				}

				case 'video':
				{
					if (this._options.bandwidth && this._options.bandwidth.video)
					{
						objMedia.bandwidth =
						[
							{ type: 'AS', limit : this._options.bandwidth.video }
						];
					}
					break;
				}
			}

			// Add the media section.
			obj.media.push(objMedia);
		}

		// Create local RTCSessionDescription.
		let localDescription =
		{
			type : type,
			sdp  : sdpTransform.write(obj)
		};

		return localDescription;
	}

	_createLocalDescriptionPlanB(type)
	{
		this._logger.debug('_createLocalDescriptionPlanB() [type:%s]', type);

		let obj = {};

		obj.version = 0;
		obj.origin =
		{
			address        : '0.0.0.0',
			ipVer          : 4,
			netType        : 'IN',
			sessionId      : this._sdpFields.id,
			sessionVersion : this._sdpFields.version,
			username       : 'mediasoup-webrtc'
		};
		obj.name = '-';
		obj.timing = { start: 0, stop: 0 };
		obj.icelite = 'ice-lite';
		obj.msidSemantic =
		{
			semantic : 'WMS',
			token    : '*'
		};
		obj.groups =
		[
			{
				type : 'BUNDLE',
				mids : Array.from(this._rtpHandlers.keys())
					.filter((mid) =>
					{
						let rtpHandler = this._rtpHandlers.get(mid);

						return rtpHandler.isRtpReceiver();
					})
					.join(' ')
			}
		];
		obj.media = [];

		let transport = this._peer.transports[0];

		// DTLS fingerprint.
		obj.fingerprint =
		{
			type : 'sha-256',
			hash : extra.fingerprintToSDP(transport.dtlsLocalParameters.fingerprints['sha-256'])
		};

		// Map
		// - key   : kind
		// - value : transceiver (object)
		//   - kind      : Media kind
		//   - mid       : MID
		//   - transport : Transport
		//   - mapCodecs : Map
		//     - key   : payload type
		//     - value : RtcCodecParameters
		//   - mapMediaSsrcs : Map
		//     - key   : SSRC
		//     - value : Object
		//       - ssrc  : SSRC
		//       - cname : CNAME
		//       - msid  : msid
		let mapKindTransceiver = new Map();

		for (let rtpHandler of this._rtpHandlers.values())
		{
			let kind = rtpHandler.kind;
			let isClosedRtpSender = false;

			if (rtpHandler.isRtpSender() &&
				(rtpHandler.closed || !rtpHandler.available))
			{
				isClosedRtpSender = true;
			}

			if (!mapKindTransceiver.has(kind))
			{
				if (rtpHandler.isRtpSender())
					continue;

				mapKindTransceiver.set(kind,
					{
						kind          : kind,
						mid           : rtpHandler.rtpParameters.muxId,
						transport     : rtpHandler.transport,
						mapCodecs     : new Map(),
						mapMediaSsrcs : new Map()
					});
			}

			let transceiver = mapKindTransceiver.get(kind);

			// Get codecs.
			for (let codec of rtpHandler.rtpParameters.codecs)
			{
				transceiver.mapCodecs.set(codec.payloadType, codec);
			}

			// Get SSRC (just for available RtpSender).
			if (rtpHandler.isRtpSender() && !isClosedRtpSender)
			{
				for (let encoding of rtpHandler.rtpParameters.encodings)
				{
					if (encoding.ssrc)
					{
						transceiver.mapMediaSsrcs.set(encoding.ssrc,
							{
								ssrc  : encoding.ssrc,
								cname : rtpHandler.rtpParameters.rtcp.cname,
								msid  : rtpHandler.rtpParameters.userParameters.msid
							});
					}
				}
			}
		}

		for (let transceiver of mapKindTransceiver.values())
		{
			let objMedia = {};
			let kind = transceiver.kind;
			let mid = transceiver.mid;
			let transport = transceiver.transport;
			let mapCodecs = transceiver.mapCodecs;
			let mapMediaSsrcs = transceiver.mapMediaSsrcs;

			// m= line.
			objMedia.type = kind;
			objMedia.port = fakePort++;  // Fuck you SDP
			if (fakePort > 60000)
				fakePort = 10000;
			objMedia.protocol = 'RTP/SAVPF';

			// c= line.
			objMedia.connection = { ip: '0.0.0.0', version: 4 };  // Fuck you SDP

			// mid.
			objMedia.mid = mid;

			// ICE.
			objMedia.iceUfrag = transport.iceLocalParameters.usernameFragment;
			objMedia.icePwd = transport.iceLocalParameters.password;
			objMedia.candidates = [];

			for (let candidate of transport.iceLocalCandidates)
			{
				let objCandidate = {};

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
					objCandidate.tcpType = candidate.tcpType;

				objMedia.candidates.push(objCandidate);
			}

			// DTLS.

			// If 'offer' always use 'actpass'.
			if (type === 'offer')
				objMedia.setup = 'actpass';
			else
				objMedia.setup = transport.dtlsLocalParameters.role === 'client' ? 'active' : 'passive';

			// Media.

			objMedia.direction = 'sendrecv';

			// Array of payload types.
			let payloads = [];

			objMedia.rtp = [];
			objMedia.rtcpFb = [];
			objMedia.fmtp = [];

			for (let codec of mapCodecs.values())
			{
				payloads.push(codec.payloadType);

				let codecSubtype = codec.name.split('/')[1];
				let rtp =
				{
					payload : codec.payloadType,
					codec   : codecSubtype,
					rate    : codec.clockRate
				};

				if (codec.numChannels > 1)
					rtp.encoding = codec.numChannels;

				objMedia.rtp.push(rtp);

				// If the codec has RTX add another SDP codec and a corresponding
				// fmtp entry.
				if (codec.rtx)
				{
					payloads.push(codec.rtx.payloadType);

					objMedia.rtp.push(
						{
							payload : codec.rtx.payloadType,
							codec   : 'rtx',
							rate    : codec.clockRate
						});

					let rtxFmtp =
					{
						payload : codec.rtx.payloadType,
						config  : `apt=${codec.payloadType}`
					};

					if (codec.rtx.rtxTime)
						rtxFmtp.config += `;rtx-time=${codec.rtx.rtxTime}`;

					objMedia.fmtp.push(rtxFmtp);
				}

				// If codec has parameters add them into fmtp.
				if (codec.parameters)
				{
					let paramFmtp =
					{
						payload : codec.payloadType,
						config  : ''
					};

					for (let key in codec.parameters)
					{
						if (codec.parameters.hasOwnProperty(key))
						{
							let normalizedKey = extra.paramToSDP(key);

							if (paramFmtp.config)
								paramFmtp.config += ';';
							paramFmtp.config += `${normalizedKey}=${codec.parameters[key]}`;
						}
					}

					if (paramFmtp.config)
						objMedia.fmtp.push(paramFmtp);
				}

				if (codec.rtcpFeedback)
				{
					for (let fb of codec.rtcpFeedback)
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

			objMedia.payloads = payloads.join(' ');
			objMedia.ssrcs = [];

			for (let mediaSsrc of mapMediaSsrcs.values())
			{
				objMedia.ssrcs.push(
					{
						id        : mediaSsrc.ssrc,
						attribute : 'cname',
						value     : mediaSsrc.cname
					});

				objMedia.ssrcs.push(
					{
						id        : mediaSsrc.ssrc,
						attribute : 'msid',
						value     : mediaSsrc.msid
					});
			}

			// RTCP.

			// Set rtcp-mux.
			objMedia.rtcpMux = 'rtcp-mux';

			// Bandwidth.
			switch (kind)
			{
				case 'audio':
				{
					if (this._options.bandwidth && this._options.bandwidth.audio)
					{
						objMedia.bandwidth =
						[
							{ type: 'AS', limit : this._options.bandwidth.audio }
						];
					}
					break;
				}

				case 'video':
				{
					if (this._options.bandwidth && this._options.bandwidth.video)
					{
						objMedia.bandwidth =
						[
							{ type: 'AS', limit : this._options.bandwidth.video }
						];
					}
					break;
				}
			}

			// Add the media section.
			obj.media.push(objMedia);
		}

		// Create local RTCSessionDescription.
		let localDescription =
		{
			type : type,
			sdp  : sdpTransform.write(obj)
		};

		return localDescription;
	}
}

module.exports = RTCPeerConnection;
