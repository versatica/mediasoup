'use strict';

const sdpTransform = require('sdp-transform');
const extra = require('../../extra');

module.exports =
{
	descToCapabilities(parsed)
	{
		// Map of RtpCodecParameters indexed by payload type.
		let mapCodecs = new Map();
		// RtpHeaderExtensions.
		let headerExtensions = [];

		for (let parsedMedia of parsed.media)
		{
			// Media kind.
			let kind = parsedMedia.type;

			for (let rtp of parsedMedia.rtp)
			{
				// Ignore feature codecs.
				switch (rtp.codec.toLowerCase())
				{
					case 'rtx':
					case 'ulpfec':
					case 'flexfec':
					case 'red':
						continue;
					default:
				}

				// TODO: This will fail for kind:depth and video/vp8, but who knows?
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

			if (Array.isArray(parsedMedia.ext))
			{
				for (let ext of parsedMedia.ext)
				{
					let preferredId = ext.value;
					let uri = ext.uri;
					let headerExtension =
					{
						kind        : kind,
						uri         : uri,
						preferredId : preferredId
					};

					// Don't duplicate.
					let duplicated = false;

					for (let i=0, len=headerExtensions.length; i<len; i++)
					{
						let savedHeaderExtension = headerExtensions[i];

						if (
							headerExtension.kind === savedHeaderExtension.kind &&
							headerExtension.uri === savedHeaderExtension.uri)
						{
							duplicated = true;
							break;
						}
					}

					if (!duplicated)
						headerExtensions.push(headerExtension);
				}
			}
		}

		let capabilities =
		{
			codecs           : Array.from(mapCodecs.values()),
			headerExtensions : headerExtensions,
			fecMechanisms    : []  // TODO
		};

		return capabilities;
	},

	descToRemoteDtlsParameters(parsed)
	{
		let media;

		// If the first media (i.e. audio) is inactive, it has no DTLS stuff so iterate.
		for (let m of parsed.media)
		{
			if (m.setup)
			{
				media = m;
				break;
			}
		}

		if (!media)
			return null;

		// NOTE: Firefox uses global fingerprint attribute.
		let remoteFingerprint = media.fingerprint || parsed.fingerprint;
		let remoteDtlsRole;

		switch (media.setup)
		{
			case 'active':
				remoteDtlsRole = 'client';
				break;
			case 'passive':
				remoteDtlsRole = 'server';
				break;
		}

		let remoteDtlsParameters =
		{
			role        : remoteDtlsRole,
			fingerprint :
			{
				algorithm : remoteFingerprint.type,
				value     : extra.fingerprintFromSDP(remoteFingerprint.hash)
			}
		};

		return remoteDtlsParameters;
	},

	descToRtpHeaderExtensionParameters(parsedMedia)
	{
		let headerExtensions = [];

		if (!Array.isArray(parsedMedia.ext))
			return headerExtensions;

		for (let ext of parsedMedia.ext)
		{
			let id = ext.value;
			let uri = ext.uri;

			headerExtensions.push(
				{
					uri : uri,
					id  : id
				});
		}

		return headerExtensions;
	}
};
