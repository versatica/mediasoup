'use strict';

const sdpTransform = require('sdp-transform');
const extra = require('../../extra');

module.exports =
{
	descToCapabilities(parsed)
	{
		// Map of RtpCodecParameters indexed by payload type.
		let mapCodecs = new Map();
		// Map of RtpHeaderExtension indexed by preferredId.
		let mapHeaderExtensions = new Map();

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

					for (let key in Object.keys(params))
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

					// TODO: Hack for https://github.com/clux/sdp-transform/issues/51
					if (typeof preferredId !== 'number')
						preferredId = parseInt(preferredId);

					let headerExtension =
					{
						kind        : kind,
						uri         : uri,
						preferredId : preferredId
					};

					// If same preferredId was already in the map with a different kind,
					// assume 'all' kinds.
					let existingHeaderExtension = mapHeaderExtensions.get(headerExtension.preferredId);

					if (existingHeaderExtension)
					{
						if (existingHeaderExtension.kind !== headerExtension.kind)
						{
							headerExtension.kind = '';
							mapHeaderExtensions.set(headerExtension.preferredId, headerExtension);
						}
					}
					else
					{
						mapHeaderExtensions.set(headerExtension.preferredId, headerExtension);
					}
				}
			}
		}

		// TODO: REMOVE
		// console.warn('------ RTP HEADER EXTENSIONS:\n%s', JSON.stringify(Array.from(mapHeaderExtensions.values()), null, '\t'));

		let capabilities =
		{
			codecs           : Array.from(mapCodecs.values()),
			headerExtensions : Array.from(mapHeaderExtensions.values()),
			fecMechanisms    : []  // TODO
		};

		return capabilities;
	},

	descToRemoteDtlsParameters(parsed)
	{
		let firstMedia = parsed.media[0];
		// NOTE: Firefox uses global fingerprint attribute.
		let remoteFingerprint = firstMedia.fingerprint || parsed.fingerprint;
		let remoteDtlsRole;

		switch (firstMedia.setup)
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
	}
};
