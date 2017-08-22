'use strict';

const sdpTransform = require('sdp-transform');

module.exports =
{
	descToCapabilities(parsed)
	{
		// Map of RtpCodecParameters indexed by payload type.
		const mapCodecs = new Map();
		// RtpHeaderExtensions.
		const headerExtensions = [];

		for (const parsedMedia of parsed.media)
		{
			// Media kind.
			const kind = parsedMedia.type;

			for (const rtp of parsedMedia.rtp)
			{
				// Ignore feature codecs.
				switch (rtp.codec.toLowerCase())
				{
					case 'ulpfec':
					case 'flexfec':
					case 'red':
						continue;
					default:
				}

				// TODO: This will fail for kind:depth and video/vp8, but who knows?
				const codecMime = `${kind}/${rtp.codec}`;
				const codec =
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

			if (Array.isArray(parsedMedia.ext))
			{
				for (const ext of parsedMedia.ext)
				{
					const preferredId = ext.value;
					const uri = ext.uri;
					const headerExtension =
					{
						kind        : kind,
						uri         : uri,
						preferredId : preferredId
					};

					// Don't duplicate.
					let duplicated = false;

					for (let i=0, len=headerExtensions.length; i<len; i++)
					{
						const savedHeaderExtension = headerExtensions[i];

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

		const capabilities =
		{
			codecs           : Array.from(mapCodecs.values()),
			headerExtensions : headerExtensions,
			fecMechanisms    : [] // TODO
		};

		return capabilities;
	},

	descToRemoteDtlsParameters(parsed)
	{
		let media;

		// If the first media (i.e. audio) is inactive, it has no DTLS stuff so iterate.
		for (const m of parsed.media)
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
		const remoteFingerprint = media.fingerprint || parsed.fingerprint;
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

		const remoteDtlsParameters =
		{
			role        : remoteDtlsRole,
			fingerprint :
			{
				algorithm : remoteFingerprint.type,
				value     : remoteFingerprint.hash
			}
		};

		return remoteDtlsParameters;
	},

	descToRtpHeaderExtensionParameters(parsedMedia)
	{
		const headerExtensions = [];

		if (!Array.isArray(parsedMedia.ext))
			return headerExtensions;

		for (const ext of parsedMedia.ext)
		{
			const id = ext.value;
			const uri = ext.uri;

			headerExtensions.push(
				{
					uri : uri,
					id  : id
				});
		}

		return headerExtensions;
	},

	/**
	 * This assumes no FEC.
	 */
	getPlanBSsrcs(parsedMedia)
	{
		const singleSsrcs = new Set();
		const simulcastSsrcs = new Set();
		const rtxSsrcs = new Set();
		const ssrcs = [];

		// Simulcast/RTX.
		if (Array.isArray(parsedMedia.ssrcGroups))
		{
			for (const group of parsedMedia.ssrcGroups)
			{
				switch (group.semantics)
				{
					case 'SIM':
					{
						const simulcast = [];

						for (const ssrc of group.ssrcs.split(' '))
						{
							const ssrc2 = Number(ssrc);

							simulcastSsrcs.add(ssrc2);
							simulcast.push(ssrc2);
						}

						ssrcs.push(simulcast);

						break;
					}

					case 'FID':
					{
						const rtx = [];

						for (const ssrc of group.ssrcs.split(' '))
						{
							const ssrc2 = Number(ssrc);

							rtxSsrcs.add(ssrc2);
							rtx.push(ssrc2);
						}

						ssrcs.push(rtx);

						break;
					}
				}
			}
		}

		// Single streams.
		if (Array.isArray(parsedMedia.ssrcs))
		{
			for (const ssrcObj of parsedMedia.ssrcs)
			{
				const ssrc = ssrcObj.id;

				if (simulcastSsrcs.has(ssrc) || singleSsrcs.has(ssrc) || rtxSsrcs.has(ssrc))
					continue;

				singleSsrcs.add(ssrc);
				ssrcs.push(ssrc);
			}
		}

		return ssrcs;
	},

	getPlanBTrackInfo(parsedMedia, ssrc)
	{
		const info =
		{
			cname   : null,
			msid    : null,
			trackId : null
		};

		if (!Array.isArray(parsedMedia.ssrcs))
			return info;

		// If the given ssrc is a simulcast array of ssrcs, just take the first one.
		if (Array.isArray(ssrc))
			ssrc = ssrc[0];

		for (const ssrcObj of parsedMedia.ssrcs)
		{
			if (ssrcObj.id !== ssrc)
				continue;

			// Chrome uses:
			//   a=ssrc:xxxx msid:yyyy zzzz
			//   a=ssrc:xxxx mslabel:yyyy
			//   a=ssrc:xxxx label:zzzz
			// Where yyyy is the MediaStream.id and zzzz the MediaStreamTrack.id.
			switch (ssrcObj.attribute)
			{
				case 'cname':
					info.cname = ssrcObj.value;
					break;

				case 'msid':
					info.msid = ssrcObj.value;
					break;

				case 'label':
					info.trackId = ssrcObj.value;
					break;
			}
		}

		return info;
	}
};
