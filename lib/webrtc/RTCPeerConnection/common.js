'use strict';

const sdpTransform = require('sdp-transform');
const extra = require('../../extra');

module.exports =
{
	descToCapabilities(desc)
	{
		let parsed = desc.parsed;
		// Map of RtpCodecParameters indexed by payload type.
		let mapCodecs = new Map();

		for (let media of parsed.media)
		{
			// Media kind.
			let kind = media.type;

			for (let rtp of media.rtp)
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

			if (media.fmtp)
			{
				for (let fmtp of media.fmtp)
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

			let capabilities =
			{
				codecs           : Array.from(mapCodecs.values()),
				headerExtensions : [], // TODO
				fecMechanisms    : []  // TODO
			};

			return capabilities;
		}
	}
};
