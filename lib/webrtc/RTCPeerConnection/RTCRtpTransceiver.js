'use strict';

const utils = require('../../utils');

class RTCRtpTransceiver
{
	constructor(mid, kind)
	{
		// MID value.
		this._mid = mid;

		// Media kind.
		this._kind = kind;

		// Peer's capabilities.
		this._capabilities = null;

		// Transport instance.
		this._transport = null;

		// RtpReceiver instance.
		this._receiver = null;

		// RtpSender instance.
		this._sender = null;

		// direction attribute.
		this._direction = 'inactive';
	}

	get closed()
	{
		if (this._sender && this._sender.closed)
			return true;
		else if (this._receiver && this._receiver.closed)
			return true;
		else
			return false;
	}

	get mid()
	{
		return this._mid;
	}

	get kind()
	{
		return this._kind;
	}

	set capabilities(capabilities)
	{
		this._capabilities = capabilities;
	}

	get transport()
	{
		return this._transport;
	}

	set transport(transport)
	{
		this._transport = transport;
	}

	get receiver()
	{
		return this._receiver;
	}

	set receiver(receiver)
	{
		if (this._sender)
			throw new Error('RtpSender already set, cannot set a RtpReceiver');

		this._receiver = receiver;
		this._direction = 'recvonly';
	}

	get sender()
	{
		return this._sender;
	}

	set sender(sender)
	{
		if (this._receiver)
			throw new Error('RtpReceiver already set, cannot set a RtpSender');

		this._sender = sender;
		if (!sender.closed && sender.active)
			this._direction = 'sendonly';
		else
			this._direction = 'inactive';
	}

	get direction()
	{
		if (this.closed)
			return 'inactive';
		else
			return this._direction;
	}

	close()
	{
		this._direction = 'inactive';

		if (this._receiver)
			this._receiver.close();

		if (this._sender)
			this._sender.close();
	}

	toObjectMedia(type)
	{
		const transport = this._transport;
		const capabilities = this._capabilities;
		const objMedia = {};
		let closed = this.closed;

		// m= line.
		objMedia.type = this._kind;
		objMedia.port = 7;
		objMedia.protocol = 'RTP/SAVPF';

		// If the sender is closed, set port 0.
		if (closed)
			objMedia.port = 0;

		// c= line.
		objMedia.connection = { ip: '127.0.0.1', version: 4 };

		// a=mid attribute.
		objMedia.mid = this._mid;

		// ICE.
		if (!closed)
		{
			objMedia.iceUfrag = transport.iceLocalParameters.usernameFragment;
			objMedia.icePwd = transport.iceLocalParameters.password;
		}

		if (!closed)
		{
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
		}

		// DTLS.
		// If 'offer' always use 'actpass'.
		if (!closed)
		{
			if (type === 'offer')
				objMedia.setup = 'actpass';
			else
				objMedia.setup = transport.dtlsLocalParameters.role === 'client' ? 'active' : 'passive';
		}

		// a=direction attribute.
		objMedia.direction = this.direction;

		objMedia.rtp = [];
		objMedia.rtcpFb = [];
		objMedia.fmtp = [];

		// Array of payload types.
		const payloads = [];
		// Codecs.
		let codecs;

		// If a RtpSender, take the codecs from it. Otherwise take them from the
		// peer's capabilities.
		if (this._sender)
			codecs = this._sender.rtpParameters.codecs;
		else
			codecs = capabilities.codecs;

		for (const codec of codecs)
		{
			if (codec.kind && codec.kind !== this._kind)
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
			closed = true;

			objMedia.payloads = '7'; // I don't care.
			objMedia.port = 0;
			objMedia.direction = 'inactive';
		}
		else
		{
			// Codec payloads.
			objMedia.payloads = payloads.join(' ');
		}

		// SSCR and CNAME if sender.
		if (this._sender && !this._sender.closed)
		{
			objMedia.ssrcs = [];

			objMedia.ssrcGroups = [];

			// a=msid attribute (used by Firefox).
			if (this._sender.rtpParameters.userParameters.msid)
				objMedia.msid = this._sender.rtpParameters.userParameters.msid;

			for (const encoding of this._sender.rtpParameters.encodings)
			{
				objMedia.ssrcs.push(
					{
						id        : encoding.ssrc,
						attribute : 'cname',
						value     : this._sender.rtpParameters.rtcp.cname
					});

				// RTX.
				if (encoding.rtx !== undefined && encoding.rtx.ssrc !== 0)
				{
					objMedia.ssrcs.push(
						{
							id        : encoding.rtx.ssrc,
							attribute : 'cname',
							value     : this._sender.rtpParameters.rtcp.cname
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
		if (!closed)
		{
			objMedia.ext = [];

			// If a RtpSender, take the header extensions from it.
			if (this._sender)
			{
				for (const headerExtension of this._sender.rtpParameters.headerExtensions)
				{
					objMedia.ext.push(
						{
							value : headerExtension.id,
							uri   : headerExtension.uri
						});
				}
			}
			// Otherwise take them from peer's capabilities.
			else
			{
				for (const headerExtension of capabilities.headerExtensions)
				{
					if (headerExtension.kind && headerExtension.kind !== this._kind)
						continue;

					objMedia.ext.push(
						{
							value : headerExtension.preferredId,
							uri   : headerExtension.uri
						});
				}
			}
		}

		// a=rtcp-mux and a=rtcp-rsize attributes.
		if (!closed)
		{
			objMedia.rtcpMux = 'rtcp-mux';
			objMedia.rtcpRsize = 'rtcp-rsize';
		}

		return objMedia;
	}
}

module.exports = RTCRtpTransceiver;
