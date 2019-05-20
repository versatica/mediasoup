const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;

expect.extend({ toBeType });

let worker;
let router1;
let router2;
let transport1;
let audioProducer;
let videoProducer;
let transport2;
let videoConsumer;

const mediaCodecs =
[
	{
		kind      : 'audio',
		mimeType  : 'audio/opus',
		clockRate : 48000,
		channels  : 2
	},
	{
		kind      : 'video',
		mimeType  : 'video/VP8',
		clockRate : 90000
	},
	{
		kind      : 'video',
		mimeType  : 'video/VP8',
		clockRate : 90000
	}
];

const audioProducerParameters =
{
	kind          : 'audio',
	rtpParameters :
	{
		mid    : 'AUDIO',
		codecs :
		[
			{
				mimeType    : 'audio/opus',
				payloadType : 111,
				clockRate   : 48000,
				channels    : 2,
				parameters  :
				{
					useinbandfec : 1,
					foo          : 'bar1'
				}
			}
		],
		headerExtensions :
		[
			{
				uri : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id  : 10
			}
		],
		encodings : [ { ssrc: 11111111 } ],
		rtcp      :
		{
			cname : 'FOOBAR'
		}
	},
	appData : { foo: 'bar1' }
};

const videoProducerParameters =
{
	kind          : 'video',
	rtpParameters :
	{
		mid    : 'VIDEO',
		codecs :
		[
			{
				mimeType     : 'video/VP8',
				payloadType  : 112,
				clockRate    : 90000,
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'goog-remb' },
					{ type: 'lalala' }
				]
			}
		],
		headerExtensions :
		[
			{
				uri : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id  : 10
			},
			{
				uri : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time',
				id  : 11
			},
			{
				uri : 'urn:3gpp:video-orientation',
				id  : 13
			}
		],
		encodings :
		[
			{ ssrc: 22222222 },
			{ ssrc: 22222223 },
			{ ssrc: 22222224 }
		],
		rtcp :
		{
			cname : 'FOOBAR'
		}
	},
	appData : { foo: 'bar2' }
};

const consumerDeviceCapabilities =
{
	codecs :
	[
		{
			mimeType             : 'audio/opus',
			kind                 : 'audio',
			clockRate            : 48000,
			preferredPayloadType : 100,
			channels             : 2
		},
		{
			mimeType             : 'video/VP8',
			kind                 : 'video',
			clockRate            : 90000,
			preferredPayloadType : 101,
			rtcpFeedback         :
			[
				{ type: 'nack' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'google-remb' },
				{ type: 'transport-cc' }
			]
		},
		{
			mimeType             : 'video/rtx',
			kind                 : 'video',
			clockRate            : 90000,
			preferredPayloadType : 102,
			rtcpFeedback         : [],
			parameters           :
			{
				apt : 101
			}
		}
	],
	headerExtensions :
	[
		{
			kind             : 'video',
			uri              : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time',
			preferredId      : 4,
			preferredEncrypt : false,
			direction        : 'sendrecv'
		},
		{
			kind             : 'video',
			uri              : 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01',
			preferredId      : 5,
			preferredEncrypt : false
		},
		{
			kind             : 'audio',
			uri              : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
			preferredId      : 10,
			preferredEncrypt : false
		}
	]
};

beforeAll(async () =>
{
	worker = await createWorker();
	router1 = await worker.createRouter({ mediaCodecs });
	router2 = await worker.createRouter({ mediaCodecs });
	transport1 = await router1.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ]
		});
	transport2 = await router2.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ]
		});
	audioProducer = await transport1.produce(audioProducerParameters);
	videoProducer = await transport1.produce(videoProducerParameters);

	// Pause the videoProducer.
	await videoProducer.pause();
});

afterAll(() => worker.close());

test('router.pipeToRouter() succeeds with audio', async () =>
{
	let dump;

	const { pipeConsumer, pipeProducer } = await router1.pipeToRouter(
		{
			producerId : audioProducer.id,
			router     : router2
		});

	dump = await router1.dump();

	// There shoud should be two Transports in router1:
	// - WebRtcTransport for audioProducer and videoProducer.
	// - PipeTransport between router1 and router2.
	expect(dump.transportIds.length).toBe(2);

	dump = await router2.dump();

	// There shoud should be two Transports in router2:
	// - WebRtcTransport for audioConsumer and videoConsumer.
	// - pipeTransport between router2 and router1.
	expect(dump.transportIds.length).toBe(2);

	expect(pipeConsumer.id).toBeType('string');
	expect(pipeConsumer.closed).toBe(false);
	expect(pipeConsumer.kind).toBe('audio');
	expect(pipeConsumer.rtpParameters).toBeType('object');
	expect(pipeConsumer.rtpParameters.mid).toBe(undefined);
	expect(pipeConsumer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType    : 'audio/opus',
				clockRate   : 48000,
				payloadType : 100,
				channels    : 2,
				parameters  :
				{
					useinbandfec : 1,
					foo          : 'bar1'
				},
				rtcpFeedback : []
			}
		]);

	expect(pipeConsumer.rtpParameters.headerExtensions).toEqual(
		[
			{
				uri : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				id  : 10
			}
		]);
	expect(pipeConsumer.rtpParameters.encodings).toEqual(
		[
			audioProducer.consumableRtpParameters.encodings[0]
		]);
	expect(pipeConsumer.type).toBe('pipe');
	expect(pipeConsumer.paused).toBe(false);
	expect(pipeConsumer.producerPaused).toBe(false);
	expect(pipeConsumer.score).toBe(undefined);
	expect(pipeConsumer.appData).toEqual({});

	expect(pipeProducer.id).toBe(audioProducer.id);
	expect(pipeProducer.closed).toBe(false);
	expect(pipeProducer.kind).toBe('audio');
	expect(pipeProducer.rtpParameters).toBeType('object');
	expect(pipeProducer.rtpParameters.mid).toBe(undefined);
	expect(pipeProducer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType    : 'audio/opus',
				clockRate   : 48000,
				payloadType : 100,
				channels    : 2,
				parameters  :
				{
					useinbandfec : 1,
					foo          : 'bar1'
				},
				rtcpFeedback : []
			}
		]);
	expect(pipeProducer.rtpParameters.headerExtensions).toEqual(
		[
			{
				uri : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				id  : 10
			}
		]);
	expect(pipeProducer.rtpParameters.encodings).toEqual(
		[
			audioProducer.consumableRtpParameters.encodings[0]
		]);
	expect(pipeProducer.paused).toBe(false);
}, 2000);

test('router.pipeToRouter() succeeds with video', async () =>
{
	let dump;

	const { pipeConsumer, pipeProducer } = await router1.pipeToRouter(
		{
			producerId : videoProducer.id,
			router     : router2
		});

	dump = await router1.dump();

	// No new PipeTransport should has been created. The existing one is used.
	expect(dump.transportIds.length).toBe(2);

	dump = await router2.dump();

	// No new PipeTransport should has been created. The existing one is used.
	expect(dump.transportIds.length).toBe(2);

	expect(pipeConsumer.id).toBeType('string');
	expect(pipeConsumer.closed).toBe(false);
	expect(pipeConsumer.kind).toBe('video');
	expect(pipeConsumer.rtpParameters).toBeType('object');
	expect(pipeConsumer.rtpParameters.mid).toBe(undefined);
	expect(pipeConsumer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType     : 'video/VP8',
				clockRate    : 90000,
				payloadType  : 101,
				rtcpFeedback :
				[
					{ type: 'nack', parameter: 'pli' },
					{ type: 'ccm', parameter: 'fir' }
				]
			}
		]);
	expect(pipeConsumer.rtpParameters.headerExtensions).toEqual(
		[
			// NOTE: Remove this once framemarking draft becomes RFC.
			{
				uri : 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07',
				id  : 6
			},
			{
				uri : 'urn:ietf:params:rtp-hdrext:framemarking',
				id  : 7
			},
			{
				uri : 'urn:3gpp:video-orientation',
				id  : 11
			},
			{
				uri : 'urn:ietf:params:rtp-hdrext:toffset',
				id  : 12
			}
		]);
	expect(pipeConsumer.rtpParameters.encodings).toEqual(
		[
			videoProducer.consumableRtpParameters.encodings[0],
			videoProducer.consumableRtpParameters.encodings[1],
			videoProducer.consumableRtpParameters.encodings[2]
		]);

	expect(pipeConsumer.type).toBe('pipe');
	expect(pipeConsumer.paused).toBe(false);
	expect(pipeConsumer.producerPaused).toBe(true);
	expect(pipeConsumer.score).toBe(undefined);
	expect(pipeConsumer.appData).toEqual({});

	expect(pipeProducer.id).toBe(videoProducer.id);
	expect(pipeProducer.closed).toBe(false);
	expect(pipeProducer.kind).toBe('video');
	expect(pipeProducer.rtpParameters).toBeType('object');
	expect(pipeProducer.rtpParameters.mid).toBe(undefined);
	expect(pipeProducer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType     : 'video/VP8',
				clockRate    : 90000,
				payloadType  : 101,
				rtcpFeedback :
				[
					{ type: 'nack', parameter: 'pli' },
					{ type: 'ccm', parameter: 'fir' }
				]
			}
		]);
	expect(pipeProducer.rtpParameters.headerExtensions).toEqual(
		[
			// NOTE: Remove this once framemarking draft becomes RFC.
			{
				uri : 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07',
				id  : 6
			},
			{
				uri : 'urn:ietf:params:rtp-hdrext:framemarking',
				id  : 7
			},
			{
				uri : 'urn:3gpp:video-orientation',
				id  : 11
			},
			{
				uri : 'urn:ietf:params:rtp-hdrext:toffset',
				id  : 12
			}
		]);
	expect(pipeProducer.paused).toBe(true);
}, 2000);

test('transport.consume() for a pipe Producer succeeds', async () =>
{
	videoConsumer = await transport2.consume(
		{
			producerId      : videoProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		});

	expect(videoConsumer.id).toBeType('string');
	expect(videoConsumer.closed).toBe(false);
	expect(videoConsumer.kind).toBe('video');
	expect(videoConsumer.rtpParameters).toBeType('object');
	expect(videoConsumer.rtpParameters.mid).toBe(undefined);
	expect(videoConsumer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType     : 'video/VP8',
				clockRate    : 90000,
				payloadType  : 101,
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'ccm', parameter: 'fir' },
					{ type: 'google-remb' }
				]
			},
			{
				mimeType     : 'video/rtx',
				clockRate    : 90000,
				payloadType  : 102,
				rtcpFeedback : [],
				parameters   :
				{
					apt : 101
				}
			}
		]);
	expect(videoConsumer.rtpParameters.headerExtensions).toEqual(
		[
			{
				uri : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time',
				id  : 4
			}
		]);
	expect(videoConsumer.rtpParameters.encodings.length).toBe(1);
	expect(videoConsumer.rtpParameters.encodings[0].ssrc).toBeType('number');
	expect(videoConsumer.rtpParameters.encodings[0].rtx).toBeType('object');
	expect(videoConsumer.rtpParameters.encodings[0].rtx.ssrc).toBeType('number');
	expect(videoConsumer.type).toBe('simulcast');
	expect(videoConsumer.paused).toBe(false);
	expect(videoConsumer.producerPaused).toBe(true);
	expect(videoConsumer.score).toEqual({ score: 10, producerScore: 0 });
	expect(videoConsumer.appData).toEqual({});
}, 2000);

test('producer.pause() and producer.resume() are transmitted to pipe Consumer', async () =>
{
	expect(videoProducer.paused).toBe(true);
	expect(videoConsumer.producerPaused).toBe(true);
	expect(videoConsumer.paused).toBe(false);

	await videoProducer.resume();

	if (videoConsumer.producerPaused)
		await new Promise((resolve) => videoConsumer.once('producerresume', resolve));

	expect(videoConsumer.producerPaused).toBe(false);
	expect(videoConsumer.paused).toBe(false);

	await videoProducer.pause();

	if (!videoConsumer.producerPaused)
		await new Promise((resolve) => videoConsumer.once('producerpause', resolve));

	expect(videoConsumer.producerPaused).toBe(true);
	expect(videoConsumer.paused).toBe(false);
}, 2000);

test('producer.close() is transmitted to pipe Consumer', async () =>
{
	await videoProducer.close();

	expect(videoProducer.closed).toBe(true);

	if (!videoConsumer.closed)
		await new Promise((resolve) => videoConsumer.once('producerclose', resolve));

	expect(videoConsumer.closed).toBe(true);
}, 2000);
