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

test('assertCapabilities() succeeds', () =>
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

	expect(ortc.assertCapabilities(deviceRtpCapabilities, routerRtpCapabilities))
		.toBe(undefined);
});

test('assertCapabilities() throws UnsupportedError if non compatible codecs', () =>
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
	let deviceRtpCapabilities;

	deviceRtpCapabilities =
	{
		codecs :
		[
			{
				kind                 : 'audio',
				name                 : 'PCMU',
				mimeType             : 'audio/PCMU',
				preferredPayloadType : 0,
				clockRate            : 8000,
				channels             : 1
			}
		]
	};

	expect(() => ortc.assertCapabilities(deviceRtpCapabilities, routerRtpCapabilities))
		.toThrow(UnsupportedError);

	deviceRtpCapabilities =
	{
		codecs :
		[
			{
				kind                 : 'video',
				name                 : 'H264',
				mimeType             : 'video/H264',
				clockRate            : 90000,
				preferredPayloadType : 103,
				parameters           :
				{
					'packetization-mode' : 0
				}
			}
		]
	};

	expect(() => ortc.assertCapabilities(deviceRtpCapabilities, routerRtpCapabilities))
		.toThrow(UnsupportedError);
});

test('getProducerRtpParametersMapping() succeeds', () =>
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

	const rtpParameters =
	{
		codecs :
		[
			{
				name         : 'H264',
				payloadType  : 111,
				clockRate    : 90000,
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'goog-remb' }
				],
				parameters :
				{
					foo                  : 1234,
					'packetization-mode' : 1
				}
			},
			{
				name        : 'rtx',
				payloadType : 112,
				clockRate   : 90000,
				parameters  :
				{
					apt : 111
				}
			}
		],
		headerExtensions :
		[
			{
				uri : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id  : 1
			}
		],
		encodings :
		[
			{ ssrc: 11111111, rtx: { ssrc: 11111112 } },
			{ ssrc: 21111111, rtx: { ssrc: 21111112 } },
			{ rid: 'high' }
		],
		rtcp :
		{
			cname : 'qwerty1234'
		}
	};

	const mapping =
		ortc.getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities);

	expect(mapping.codecs).toEqual(
		[
			{ payloadType: 111, mappedPayloadType: 101 },
			{ payloadType: 112, mappedPayloadType: 102 }
		]);

	expect(mapping.headerExtensions).toEqual(
		[
			{ id: 1, mappedId: 5 }
		]);

	expect(mapping.encodings[0].ssrc).toBe(11111111);
	expect(mapping.encodings[0].rtxSsrc).toBe(11111112);
	expect(mapping.encodings[0].rid).toBe(undefined);
	expect(mapping.encodings[0].mappedSsrc).toBeType('number');
	expect(mapping.encodings[0].mappedRtxSsrc).toBeType('number');
	expect(mapping.encodings[1].ssrc).toBe(21111111);
	expect(mapping.encodings[1].rtxSsrc).toBe(21111112);
	expect(mapping.encodings[1].rid).toBe(undefined);
	expect(mapping.encodings[1].mappedSsrc).toBeType('number');
	expect(mapping.encodings[1].mappedRtxSsrc).toBeType('number');
	expect(mapping.encodings[2].ssrc).toBe(undefined);
	expect(mapping.encodings[2].rtxSsrc).toBe(undefined);
	expect(mapping.encodings[2].rid).toBe('high');
	expect(mapping.encodings[2].mappedSsrc).toBeType('number');
	expect(mapping.encodings[2].mappedRtxSsrc).toBeType('number');
});

test('getProducerRtpParametersMapping() throws UnsupportedError if non compatible params', () =>
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

	const rtpParameters =
	{
		codecs :
		[
			{
				name         : 'VP8',
				payloadType  : 120,
				clockRate    : 90000,
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'fir' }
				]
			}
		],
		headerExtensions : [],
		encodings        :
		[
			{ ssrc: 11111111 }
		],
		rtcp :
		{
			cname : 'qwerty1234'
		}
	};

	expect(
		() => ortc.getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities))
		.toThrow(UnsupportedError);
});
