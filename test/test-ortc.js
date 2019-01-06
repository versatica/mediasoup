const { toBeType } = require('jest-tobetype');
const { UnsupportedError } = require('../lib/errors');
const ortc = require('../lib/ortc');

expect.extend({ toBeType });

test('generateRouterRtpCapabilities() succeeds', () =>
{
	const mediaCodecs =
	[
		{
			kind       : 'audio',
			name       : 'opus',
			mimeType   : 'audio/opus',
			clockRate  : 48000,
			channels   : 2,
			parameters :
			{
				useinbandfec : 1,
				foo          : 'bar'
			}
		},
		{
			kind      : 'video',
			name      : 'VP8',
			mimeType  : 'video/VP8',
			clockRate : 90000
		},
		{
			kind         : 'video',
			name         : 'H264',
			mimeType     : 'video/H264',
			clockRate    : 90000,
			rtcpFeedback : [], // Will be ignored.
			parameters   :
			{
				foo : 'bar'
			}
		}
	];

	const rtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

	expect(rtpCapabilities.codecs.length).toBe(5);

	// opus.
	expect(rtpCapabilities.codecs[0]).toEqual(
		{
			kind                 : 'audio',
			name                 : 'opus',
			mimeType             : 'audio/opus',
			preferredPayloadType : 100, // 100 is the first PT chosen.
			clockRate            : 48000,
			channels             : 2,
			rtcpFeedback         : [],
			parameters           :
			{
				useinbandfec : 1,
				foo          : 'bar'
			}
		});

	// VP8.
	expect(rtpCapabilities.codecs[1]).toEqual(
		{
			kind                 : 'video',
			name                 : 'VP8',
			mimeType             : 'video/VP8',
			preferredPayloadType : 101,
			clockRate            : 90000,
			rtcpFeedback         :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			],
			parameters : {}
		});

	// VP8 RTX.
	expect(rtpCapabilities.codecs[2]).toEqual(
		{
			kind                 : 'video',
			name                 : 'rtx',
			mimeType             : 'video/rtx',
			preferredPayloadType : 102,
			clockRate            : 90000,
			rtcpFeedback         : [],
			parameters           :
			{
				apt : 101
			}
		});

	// H264.
	expect(rtpCapabilities.codecs[3]).toEqual(
		{
			kind                 : 'video',
			name                 : 'H264',
			mimeType             : 'video/H264',
			preferredPayloadType : 103,
			clockRate            : 90000,
			rtcpFeedback         :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' }
			],
			parameters :
			{
				'packetization-mode' : 0,
				foo                  : 'bar'
			}
		});

	// H264 RTX.
	expect(rtpCapabilities.codecs[4]).toEqual(
		{
			kind                 : 'video',
			name                 : 'rtx',
			mimeType             : 'video/rtx',
			preferredPayloadType : 104,
			clockRate            : 90000,
			rtcpFeedback         : [],
			parameters           :
			{
				apt : 103
			}
		});
});

test('generateRouterRtpCapabilities() with unsupported codecs throws UnsupportedError', () =>
{
	let mediaCodecs;

	mediaCodecs =
	[
		{
			kind      : 'audio',
			name      : 'chicken',
			mimeType  : 'audio/chicken',
			clockRate : 8000,
			channels  : 4
		}
	];

	expect(() => ortc.generateRouterRtpCapabilities(mediaCodecs))
		.toThrow(UnsupportedError);

	mediaCodecs =
	[
		{
			kind      : 'audio',
			name      : 'opus',
			mimeType  : 'audio/opus',
			clockRate : 48000,
			channels  : 1
		}
	];

	expect(() => ortc.generateRouterRtpCapabilities(mediaCodecs))
		.toThrow(UnsupportedError);

	mediaCodecs =
	[
		{
			kind       : 'video',
			name       : 'H264',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 5
			}
		}
	];

	expect(() => ortc.generateRouterRtpCapabilities(mediaCodecs))
		.toThrow(UnsupportedError);
});

test('generateRouterRtpCapabilities() with too many codecs throws', () =>
{
	const mediaCodecs = [];

	for (let i = 0; i < 100; ++i)
	{
		mediaCodecs.push(
			{
				kind      : 'audio',
				name      : 'opus',
				mimeType  : 'audio/opus',
				clockRate : 48000,
				channels  : 2
			});
	}

	expect(() => ortc.generateRouterRtpCapabilities(mediaCodecs))
		.toThrow('cannot allocate');
});

test('assertCapabilitiesSubset() succeeds', () =>
{
	const mediaCodecs =
	[
		{
			kind      : 'audio',
			name      : 'opus',
			mimeType  : 'audio/opus',
			clockRate : 48000,
			channels  : 2
		},
		{
			kind      : 'video',
			name      : 'VP8',
			mimeType  : 'video/VP8',
			clockRate : 90000
		},
		{
			kind       : 'video',
			name       : 'H264',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 1
			}
		}
	];

	const routerRtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

	const deviceRtpCapabilities =
	{
		codecs :
		[
			{
				kind                 : 'audio',
				name                 : 'opus',
				mimeType             : 'audio/opus',
				preferredPayloadType : 100,
				clockRate            : 48000,
				channels             : 2
			},
			{
				kind                 : 'video',
				name                 : 'H264',
				mimeType             : 'video/H264',
				preferredPayloadType : 103,
				clockRate            : 90000,
				parameters           :
				{
					foo                  : 1234,
					'packetization-mode' : 1
				}
			}
		]
	};

	expect(ortc.assertCapabilitiesSubset(deviceRtpCapabilities, routerRtpCapabilities))
		.toBe(undefined);
});
