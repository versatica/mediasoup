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
let dataProducer;
let dataConsumer;

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

const dataProducerParameters =
{
	sctpStreamParameters :
	{
		streamId          : 666,
		ordered           : false,
		maxPacketLifeTime : 5000
	},
	label    : 'foo',
	protocol : 'bar'
};

const consumerDeviceCapabilities =
{
	codecs :
	[
		{
			kind                 : 'audio',
			mimeType             : 'audio/opus',
			preferredPayloadType : 100,
			clockRate            : 48000,
			channels             : 2
		},
		{
			kind                 : 'video',
			mimeType             : 'video/VP8',
			preferredPayloadType : 101,
			clockRate            : 90000,
			rtcpFeedback         :
			[
				{ type: 'nack' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'google-remb' },
				{ type: 'transport-cc' }
			]
		},
		{
			kind                 : 'video',
			mimeType             : 'video/rtx',
			preferredPayloadType : 102,
			clockRate            : 90000,
			parameters           :
			{
				apt : 101
			},
			rtcpFeedback : []
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
	],
	sctpCapabilities :
	{
		numSctpStreams : 2048
	}
};

beforeAll(async () =>
{
	worker = await createWorker();
	router1 = await worker.createRouter({ mediaCodecs });
	router2 = await worker.createRouter({ mediaCodecs });
	transport1 = await router1.createWebRtcTransport(
		{
			listenIps  : [ '127.0.0.1' ],
			enableSctp : true
		});
	transport2 = await router2.createWebRtcTransport(
		{
			listenIps  : [ '127.0.0.1' ],
			enableSctp : true
		});
	audioProducer = await transport1.produce(audioProducerParameters);
	videoProducer = await transport1.produce(videoProducerParameters);
	dataProducer = await transport1.produceData(dataProducerParameters);

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

	// There shoud be two Transports in router1:
	// - WebRtcTransport for audioProducer and videoProducer.
	// - PipeTransport between router1 and router2.
	expect(dump.transportIds.length).toBe(2);

	dump = await router2.dump();

	// There shoud be two Transports in router2:
	// - WebRtcTransport for audioConsumer and videoConsumer.
	// - pipeTransport between router2 and router1.
	expect(dump.transportIds.length).toBe(2);

	expect(pipeConsumer.id).toBeType('string');
	expect(pipeConsumer.closed).toBe(false);
	expect(pipeConsumer.kind).toBe('audio');
	expect(pipeConsumer.rtpParameters).toBeType('object');
	expect(pipeConsumer.rtpParameters.mid).toBeUndefined();
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
				uri        : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				id         : 10,
				encrypt    : false,
				parameters : {}
			}
		]);
	expect(pipeConsumer.type).toBe('pipe');
	expect(pipeConsumer.paused).toBe(false);
	expect(pipeConsumer.producerPaused).toBe(false);
	expect(pipeConsumer.score).toEqual(
		{ score: 10, producerScore: 10, producerScores: [] });
	expect(pipeConsumer.appData).toEqual({});

	expect(pipeProducer.id).toBe(audioProducer.id);
	expect(pipeProducer.closed).toBe(false);
	expect(pipeProducer.kind).toBe('audio');
	expect(pipeProducer.rtpParameters).toBeType('object');
	expect(pipeProducer.rtpParameters.mid).toBeUndefined();
	expect(pipeProducer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType    : 'audio/opus',
				payloadType : 100,
				clockRate   : 48000,
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
				uri        : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				id         : 10,
				encrypt    : false,
				parameters : {}
			}
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
	expect(pipeConsumer.rtpParameters.mid).toBeUndefined();
	expect(pipeConsumer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType     : 'video/VP8',
				payloadType  : 101,
				clockRate    : 90000,
				parameters   : {},
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
				uri        : 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07',
				id         : 6,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:framemarking',
				id         : 7,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:3gpp:video-orientation',
				id         : 11,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:toffset',
				id         : 12,
				encrypt    : false,
				parameters : {}
			}
		]);

	expect(pipeConsumer.type).toBe('pipe');
	expect(pipeConsumer.paused).toBe(false);
	expect(pipeConsumer.producerPaused).toBe(true);
	expect(pipeConsumer.score).toEqual(
		{ score: 10, producerScore: 10, producerScores: [] });
	expect(pipeConsumer.appData).toEqual({});

	expect(pipeProducer.id).toBe(videoProducer.id);
	expect(pipeProducer.closed).toBe(false);
	expect(pipeProducer.kind).toBe('video');
	expect(pipeProducer.rtpParameters).toBeType('object');
	expect(pipeProducer.rtpParameters.mid).toBeUndefined();
	expect(pipeProducer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType     : 'video/VP8',
				payloadType  : 101,
				clockRate    : 90000,
				parameters   : {},
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
				uri        : 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07',
				id         : 6,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:framemarking',
				id         : 7,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:3gpp:video-orientation',
				id         : 11,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:toffset',
				id         : 12,
				encrypt    : false,
				parameters : {}
			}
		]);
	expect(pipeProducer.paused).toBe(true);
}, 2000);

test('router.createPipeTransport() with enableRtx succeeds', async () =>
{
	const pipeTransport = await router1.createPipeTransport(
		{
			listenIp  : '127.0.0.1',
			enableRtx : true
		});

	expect(pipeTransport.srtpParameters).toBeUndefined();

	// No SRTP enabled so passing srtpParameters must fail.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				cryptoSuite : 'AES_CM_128_HMAC_SHA1_80',
				keyBase64   : 'ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv'
			}
		}))
		.rejects
		.toThrow(TypeError);

	// No SRTP enabled so passing srtpParameters (even if invalid) must fail.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters : 'invalid'
		}))
		.rejects
		.toThrow(TypeError);

	const pipeConsumer =
		await pipeTransport.consume({ producerId: videoProducer.id });

	expect(pipeConsumer.id).toBeType('string');
	expect(pipeConsumer.closed).toBe(false);
	expect(pipeConsumer.kind).toBe('video');
	expect(pipeConsumer.rtpParameters).toBeType('object');
	expect(pipeConsumer.rtpParameters.mid).toBeUndefined();
	expect(pipeConsumer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType     : 'video/VP8',
				payloadType  : 101,
				clockRate    : 90000,
				parameters   : {},
				rtcpFeedback :
				[
					{ type: 'nack', parameter: '' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'ccm', parameter: 'fir' }
				]
			},
			{
				mimeType     : 'video/rtx',
				payloadType  : 102,
				clockRate    : 90000,
				parameters   : { apt: 101 },
				rtcpFeedback : []
			}
		]);
	expect(pipeConsumer.rtpParameters.headerExtensions).toEqual(
		[
			// NOTE: Remove this once framemarking draft becomes RFC.
			{
				uri        : 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07',
				id         : 6,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:framemarking',
				id         : 7,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:3gpp:video-orientation',
				id         : 11,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:toffset',
				id         : 12,
				encrypt    : false,
				parameters : {}
			}
		]);

	expect(pipeConsumer.type).toBe('pipe');
	expect(pipeConsumer.paused).toBe(false);
	expect(pipeConsumer.producerPaused).toBe(true);
	expect(pipeConsumer.score).toEqual(
		{ score: 10, producerScore: 10, producerScores: [] });
	expect(pipeConsumer.appData).toEqual({});

	pipeTransport.close();
}, 2000);

test('router.createPipeTransport() with enableSrtp succeeds', async () =>
{
	const pipeTransport = await router1.createPipeTransport(
		{
			listenIp   : '127.0.0.1',
			enableSrtp : true
		});

	expect(pipeTransport.id).toBeType('string');
	expect(pipeTransport.srtpParameters).toBeType('object');
	expect(pipeTransport.srtpParameters.keyBase64.length).toBe(40);

	// Missing srtpParameters.
	await expect(pipeTransport.connect(
		{
			ip   : '127.0.0.2',
			port : 9999
		}))
		.rejects
		.toThrow(TypeError);

	// Invalid srtpParameters.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters : 1
		}))
		.rejects
		.toThrow(TypeError);

	// Missing srtpParameters.cryptoSuite.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				keyBase64 : 'ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv'
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Missing srtpParameters.keyBase64.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				cryptoSuite : 'AES_CM_128_HMAC_SHA1_80'
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Invalid srtpParameters.cryptoSuite.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				cryptoSuite : 'FOO',
				keyBase64   : 'ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv'
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Invalid srtpParameters.cryptoSuite.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				cryptoSuite : 123,
				keyBase64   : 'ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv'
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Invalid srtpParameters.keyBase64.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				cryptoSuite : 'AES_CM_128_HMAC_SHA1_80',
				keyBase64   : []
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Valid srtpParameters.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				cryptoSuite : 'AES_CM_128_HMAC_SHA1_80',
				keyBase64   : 'ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv'
			}
		}))
		.resolves
		.toBeUndefined();

	pipeTransport.close();
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
	expect(videoConsumer.rtpParameters.mid).toBe('0');
	expect(videoConsumer.rtpParameters.codecs).toEqual(
		[
			{
				mimeType     : 'video/VP8',
				payloadType  : 101,
				clockRate    : 90000,
				parameters   : {},
				rtcpFeedback :
				[
					{ type: 'nack', parameter: '' },
					{ type: 'ccm', parameter: 'fir' },
					{ type: 'google-remb', parameter: '' },
					{ type: 'transport-cc', parameter: '' }
				]
			},
			{
				mimeType    : 'video/rtx',
				payloadType : 102,
				clockRate   : 90000,
				parameters  :
				{
					apt : 101
				},
				rtcpFeedback : []
			}
		]);
	expect(videoConsumer.rtpParameters.headerExtensions).toEqual(
		[
			{
				uri        : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time',
				id         : 4,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01',
				id         : 5,
				encrypt    : false,
				parameters : {}
			}
		]);
	expect(videoConsumer.rtpParameters.encodings.length).toBe(1);
	expect(videoConsumer.rtpParameters.encodings[0].ssrc).toBeType('number');
	expect(videoConsumer.rtpParameters.encodings[0].rtx).toBeType('object');
	expect(videoConsumer.rtpParameters.encodings[0].rtx.ssrc).toBeType('number');
	expect(videoConsumer.type).toBe('simulcast');
	expect(videoConsumer.paused).toBe(false);
	expect(videoConsumer.producerPaused).toBe(true);
	expect(videoConsumer.score).toEqual(
		{ score: 10, producerScore: 0, producerScores: [ 0, 0, 0 ] });
	expect(videoConsumer.appData).toEqual({});
}, 2000);

test('producer.pause() and producer.resume() are transmitted to pipe Consumer', async () =>
{
	// NOTE: Let's use a Promise since otherwise there may be race conditions
	// between events and await lines below.
	let promise;

	expect(videoProducer.paused).toBe(true);
	expect(videoConsumer.producerPaused).toBe(true);
	expect(videoConsumer.paused).toBe(false);

	promise = new Promise((resolve) => videoConsumer.once('producerresume', resolve));

	await videoProducer.resume();
	await promise;

	expect(videoConsumer.producerPaused).toBe(false);
	expect(videoConsumer.paused).toBe(false);

	promise = new Promise((resolve) => videoConsumer.once('producerpause', resolve));

	await videoProducer.pause();
	await promise;

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

test('router.pipeToRouter() succeeds with data', async () =>
{
	let dump;

	const { pipeDataConsumer, pipeDataProducer } = await router1.pipeToRouter(
		{
			dataProducerId : dataProducer.id,
			router         : router2
		});

	dump = await router1.dump();

	// There shoud be two Transports in router1:
	// - WebRtcTransport for audioProducer, videoProducer and dataProducer.
	// - PipeTransport between router1 and router2.
	expect(dump.transportIds.length).toBe(2);

	dump = await router2.dump();

	// There shoud be two Transports in router2:
	// - WebRtcTransport for audioConsumer, videoConsumer and dataConsumer.
	// - pipeTransport between router2 and router1.
	expect(dump.transportIds.length).toBe(2);

	expect(pipeDataConsumer.id).toBeType('string');
	expect(pipeDataConsumer.closed).toBe(false);
	expect(pipeDataConsumer.type).toBe('sctp');
	expect(pipeDataConsumer.sctpStreamParameters).toBeType('object');
	expect(pipeDataConsumer.sctpStreamParameters.streamId).toBeType('number');
	expect(pipeDataConsumer.sctpStreamParameters.ordered).toBe(false);
	expect(pipeDataConsumer.sctpStreamParameters.maxPacketLifeTime).toBe(5000);
	expect(pipeDataConsumer.sctpStreamParameters.maxRetransmits).toBeUndefined();
	expect(pipeDataConsumer.label).toBe('foo');
	expect(pipeDataConsumer.protocol).toBe('bar');

	expect(pipeDataProducer.id).toBe(dataProducer.id);
	expect(pipeDataProducer.closed).toBe(false);
	expect(pipeDataProducer.type).toBe('sctp');
	expect(pipeDataProducer.sctpStreamParameters).toBeType('object');
	expect(pipeDataProducer.sctpStreamParameters.streamId).toBeType('number');
	expect(pipeDataProducer.sctpStreamParameters.ordered).toBe(false);
	expect(pipeDataProducer.sctpStreamParameters.maxPacketLifeTime).toBe(5000);
	expect(pipeDataProducer.sctpStreamParameters.maxRetransmits).toBeUndefined();
	expect(pipeDataProducer.label).toBe('foo');
	expect(pipeDataProducer.protocol).toBe('bar');
}, 2000);

test('transport.dataConsume() for a pipe DataProducer succeeds', async () =>
{
	dataConsumer = await transport2.consumeData(
		{
			dataProducerId : dataProducer.id
		});

	expect(dataConsumer.id).toBeType('string');
	expect(dataConsumer.closed).toBe(false);
	expect(dataConsumer.type).toBe('sctp');
	expect(dataConsumer.sctpStreamParameters).toBeType('object');
	expect(dataConsumer.sctpStreamParameters.streamId).toBeType('number');
	expect(dataConsumer.sctpStreamParameters.ordered).toBe(false);
	expect(dataConsumer.sctpStreamParameters.maxPacketLifeTime).toBe(5000);
	expect(dataConsumer.sctpStreamParameters.maxRetransmits).toBeUndefined();
	expect(dataConsumer.label).toBe('foo');
	expect(dataConsumer.protocol).toBe('bar');
}, 2000);

test('dataProducer.close() is transmitted to pipe DataConsumer', async () =>
{
	await dataProducer.close();

	expect(dataProducer.closed).toBe(true);

	if (!dataConsumer.closed)
		await new Promise((resolve) => dataConsumer.once('dataproducerclose', resolve));

	expect(dataConsumer.closed).toBe(true);
}, 2000);

test('router.pipeToRouter() called twice generates a single PipeTransport pair', async () =>
{
	const routerA = await worker.createRouter({ mediaCodecs });
	const routerB = await worker.createRouter({ mediaCodecs });
	const transportA1 = await routerA.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] });
	const transportA2 = await routerA.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] });
	const audioProducer1 = await transportA1.produce(audioProducerParameters);
	const audioProducer2 = await transportA2.produce(audioProducerParameters);
	let dump;

	await Promise.all(
		[
			routerA.pipeToRouter(
				{
					producerId : audioProducer1.id,
					router     : routerB
				}),
			routerA.pipeToRouter(
				{
					producerId : audioProducer2.id,
					router     : routerB
				})
		]);

	dump = await routerA.dump();

	// There shoud be 3 Transports in routerA:
	// - WebRtcTransport for audioProducer1 and audioProducer2.
	// - PipeTransport between routerA and routerB.
	expect(dump.transportIds.length).toBe(3);

	dump = await routerB.dump();

	// There shoud be 1 Transport in routerB:
	// - PipeTransport between routerA and routerB.
	expect(dump.transportIds.length).toBe(1);
}, 2000);
