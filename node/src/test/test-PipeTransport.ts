// @ts-ignore
import * as pickPort from 'pick-port';
import * as mediasoup from '../';

let worker1: mediasoup.types.Worker;
let worker2: mediasoup.types.Worker;
let router1: mediasoup.types.Router;
let router2: mediasoup.types.Router;
let transport1: mediasoup.types.WebRtcTransport;
let audioProducer: mediasoup.types.Producer;
let videoProducer: mediasoup.types.Producer;
let transport2: mediasoup.types.WebRtcTransport;
let videoConsumer: mediasoup.types.Consumer;
let dataProducer: mediasoup.types.DataProducer;
let dataConsumer: mediasoup.types.DataConsumer;

const mediaCodecs: mediasoup.types.RtpCodecCapability[] =
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
	}
];

const audioProducerParameters: mediasoup.types.ProducerOptions =
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

const videoProducerParameters: mediasoup.types.ProducerOptions =
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

const dataProducerParameters: mediasoup.types.DataProducerOptions =
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

const consumerDeviceCapabilities: mediasoup.types.RtpCapabilities =
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
	]
};

beforeAll(async () =>
{
	worker1 = await mediasoup.createWorker();
	worker2 = await mediasoup.createWorker();
	router1 = await worker1.createRouter({ mediaCodecs });
	router2 = await worker2.createRouter({ mediaCodecs });
	transport1 = await router1.createWebRtcTransport(
		{
			listenInfos : [ { protocol: 'udp', ip: '127.0.0.1' } ],
			enableSctp  : true
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

afterAll(() =>
{
	worker1.close();
	worker2.close();
});

test('router.pipeToRouter() succeeds with audio', async () =>
{
	let dump;

	const { pipeConsumer, pipeProducer } = await router1.pipeToRouter(
		{
			producerId : audioProducer.id,
			router     : router2
		}) as {
			pipeConsumer: mediasoup.types.Consumer;
			pipeProducer: mediasoup.types.Producer;
		};

	dump = await router1.dump();

	// There should be two Transports in router1:
	// - WebRtcTransport for audioProducer and videoProducer.
	// - PipeTransport between router1 and router2.
	expect(dump.transportIds.length).toBe(2);

	dump = await router2.dump();

	// There should be two Transports in router2:
	// - WebRtcTransport for audioConsumer and videoConsumer.
	// - PipeTransport between router2 and router1.
	expect(dump.transportIds.length).toBe(2);

	expect(typeof pipeConsumer.id).toBe('string');
	expect(pipeConsumer.closed).toBe(false);
	expect(pipeConsumer.kind).toBe('audio');
	expect(typeof pipeConsumer.rtpParameters).toBe('object');
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
			},
			{
				uri        : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time',
				id         : 13,
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
	expect(typeof pipeProducer.rtpParameters).toBe('object');
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
			},
			{
				uri        : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time',
				id         : 13,
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
		}) as {
			pipeConsumer: mediasoup.types.Consumer;
			pipeProducer: mediasoup.types.Producer;
		};

	dump = await router1.dump();

	// No new PipeTransport should has been created. The existing one is used.
	expect(dump.transportIds.length).toBe(2);

	dump = await router2.dump();

	// No new PipeTransport should has been created. The existing one is used.
	expect(dump.transportIds.length).toBe(2);

	expect(typeof pipeConsumer.id).toBe('string');
	expect(pipeConsumer.closed).toBe(false);
	expect(pipeConsumer.kind).toBe('video');
	expect(typeof pipeConsumer.rtpParameters).toBe('object');
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
			},
			{
				uri        : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time',
				id         : 13,
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
	expect(typeof pipeProducer.rtpParameters).toBe('object');
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
			},
			{
				uri        : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time',
				id         : 13,
				encrypt    : false,
				parameters : {}
			}
		]);
	expect(pipeProducer.paused).toBe(true);
}, 2000);

test('router.createPipeTransport() with wrong arguments rejects with TypeError', async () =>
{
	// @ts-ignore
	await expect(router1.createPipeTransport({}))
		.rejects
		.toThrow(TypeError);

	await expect(router1.createPipeTransport({ listenIp: '123' }))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(router1.createPipeTransport({ listenIp: [ '127.0.0.1' ] }))
		.rejects
		.toThrow(TypeError);

	await expect(router1.createPipeTransport(
		{
			listenInfo : { protocol: 'tcp', ip: '127.0.0.1' }
		}))
		.rejects
		.toThrow(TypeError);

	await expect(router1.createPipeTransport(
		{
			listenInfo : { protocol: 'udp', ip: '127.0.0.1' },
			// @ts-ignore
			appData    : 'NOT-AN-OBJECT'
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('router.createPipeTransport() with enableRtx succeeds', async () =>
{
	const pipeTransport = await router1.createPipeTransport(
		{
			listenInfo : { protocol: 'udp', ip: '127.0.0.1' },
			enableRtx  : true
		});

	const pipeConsumer =
		await pipeTransport.consume({ producerId: videoProducer.id });

	expect(typeof pipeConsumer.id).toBe('string');
	expect(pipeConsumer.closed).toBe(false);
	expect(pipeConsumer.kind).toBe('video');
	expect(typeof pipeConsumer.rtpParameters).toBe('object');
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
			},
			{
				uri        : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time',
				id         : 13,
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

test('pipeTransport.connect() with valid SRTP parameters succeeds', async () =>
{
	const pipeTransport = await router1.createPipeTransport(
		{
			listenIp   : '127.0.0.1',
			enableSrtp : true
		});

	expect(typeof pipeTransport.srtpParameters).toBe('object');
	// The master length of AEAD_AES_256_GCM.
	expect(pipeTransport.srtpParameters?.keyBase64.length).toBe(60);

	// Valid srtpParameters.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				cryptoSuite : 'AEAD_AES_256_GCM',
				keyBase64   : 'YTdjcDBvY2JoMGY5YXNlNDc0eDJsdGgwaWRvNnJsamRrdG16aWVpZHphdHo='
			}
		}))
		.resolves
		.toBeUndefined();

	pipeTransport.close();
}, 2000);

test('pipeTransport.connect() with srtpParameters fails if enableSrtp is unset', async () =>
{
	const pipeTransport = await router1.createPipeTransport(
		{
			listenInfo : { protocol: 'udp', ip: '127.0.0.1' },
			enableRtx  : true
		});

	expect(pipeTransport.srtpParameters).toBeUndefined();

	// No SRTP enabled so passing srtpParameters must fail.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
				{
					cryptoSuite : 'AEAD_AES_256_GCM',
					keyBase64   : 'YTdjcDBvY2JoMGY5YXNlNDc0eDJsdGgwaWRvNnJsamRrdG16aWVpZHphdHo='
				}
		}))
		.rejects
		.toThrow(TypeError);

	// No SRTP enabled so passing srtpParameters (even if invalid) must fail.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			// @ts-ignore
			srtpParameters : 'invalid'
		}))
		.rejects
		.toThrow(TypeError);

	pipeTransport.close();
});

test('pipeTransport.connect() with invalid srtpParameters fails', async () =>
{
	const pipeTransport = await router1.createPipeTransport(
		{
			listenIp   : '127.0.0.1',
			enableSrtp : true
		});

	expect(typeof pipeTransport.id).toBe('string');
	expect(typeof pipeTransport.srtpParameters).toBe('object');
	// The master length of AEAD_AES_256_GCM.
	expect(pipeTransport.srtpParameters?.keyBase64.length).toBe(60);

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
			// @ts-ignore
			srtpParameters : 1
		}))
		.rejects
		.toThrow(TypeError);

	// Missing srtpParameters.cryptoSuite.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			// @ts-ignore
			srtpParameters :
			{
				keyBase64 : 'YTdjcDBvY2JoMGY5YXNlNDc0eDJsdGgwaWRvNnJsamRrdG16aWVpZHphdHo='
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Missing srtpParameters.keyBase64.
	await expect(pipeTransport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			// @ts-ignore
			srtpParameters :
			{
				cryptoSuite : 'AEAD_AES_256_GCM'
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
				// @ts-ignore
				cryptoSuite : 'FOO',
				keyBase64   : 'YTdjcDBvY2JoMGY5YXNlNDc0eDJsdGgwaWRvNnJsamRrdG16aWVpZHphdHo='
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
				// @ts-ignore
				cryptoSuite : 123,
				keyBase64   : 'YTdjcDBvY2JoMGY5YXNlNDc0eDJsdGgwaWRvNnJsamRrdG16aWVpZHphdHo='
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
				cryptoSuite : 'AEAD_AES_256_GCM',
				// @ts-ignore
				keyBase64   : []
			}
		}))
		.rejects
		.toThrow(TypeError);

	pipeTransport.close();
}, 2000);

test('router.createPipeTransport() with fixed port succeeds', async () =>
{
	const port = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const pipeTransport = await router1.createPipeTransport(
		{
			listenInfo : { protocol: 'udp', ip: '127.0.0.1', port }
		});

	expect(pipeTransport.tuple.localPort).toEqual(port);

	pipeTransport.close();
}, 2000);

test('transport.consume() for a pipe Producer succeeds', async () =>
{
	videoConsumer = await transport2.consume(
		{
			producerId      : videoProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		});

	expect(typeof videoConsumer.id).toBe('string');
	expect(videoConsumer.closed).toBe(false);
	expect(videoConsumer.kind).toBe('video');
	expect(typeof videoConsumer.rtpParameters).toBe('object');
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
	expect(videoConsumer.rtpParameters.encodings?.length).toBe(1);
	expect(typeof videoConsumer.rtpParameters.encodings?.[0].ssrc).toBe('number');
	expect(typeof videoConsumer.rtpParameters.encodings?.[0].rtx).toBe('object');
	expect(typeof videoConsumer.rtpParameters.encodings?.[0].rtx?.ssrc).toBe('number');
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

	promise = new Promise<void>((resolve) => videoConsumer.once('producerresume', resolve));

	await videoProducer.resume();
	await promise;

	expect(videoConsumer.producerPaused).toBe(false);
	expect(videoConsumer.paused).toBe(false);

	promise = new Promise<void>((resolve) => videoConsumer.once('producerpause', resolve));

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
	{
		await new Promise<void>((resolve) => videoConsumer.once('producerclose', resolve));
	}

	expect(videoConsumer.closed).toBe(true);
}, 2000);

test('router.pipeToRouter() fails if both Routers belong to the same Worker', async () =>
{
	const router1bis = await worker1.createRouter({ mediaCodecs });

	await expect(router1.pipeToRouter(
		{
			producerId : videoProducer.id,
			router     : router1bis
		}))
		.rejects
		.toThrow(Error);

	router1bis.close();
}, 2000);

test('router.pipeToRouter() succeeds with data', async () =>
{
	let dump;

	const { pipeDataConsumer, pipeDataProducer } = await router1.pipeToRouter(
		{
			dataProducerId : dataProducer.id,
			router         : router2
		}) as {
			pipeDataConsumer: mediasoup.types.DataConsumer;
			pipeDataProducer: mediasoup.types.DataProducer;
		};

	dump = await router1.dump();

	// There should be two Transports in router1:
	// - WebRtcTransport for audioProducer, videoProducer and dataProducer.
	// - PipeTransport between router1 and router2.
	expect(dump.transportIds.length).toBe(2);

	dump = await router2.dump();

	// There should be two Transports in router2:
	// - WebRtcTransport for audioConsumer, videoConsumer and dataConsumer.
	// - PipeTransport between router2 and router1.
	expect(dump.transportIds.length).toBe(2);

	expect(typeof pipeDataConsumer.id).toBe('string');
	expect(pipeDataConsumer.closed).toBe(false);
	expect(pipeDataConsumer.type).toBe('sctp');
	expect(typeof pipeDataConsumer.sctpStreamParameters).toBe('object');
	expect(typeof pipeDataConsumer.sctpStreamParameters?.streamId).toBe('number');
	expect(pipeDataConsumer.sctpStreamParameters?.ordered).toBe(false);
	expect(pipeDataConsumer.sctpStreamParameters?.maxPacketLifeTime).toBe(5000);
	expect(pipeDataConsumer.sctpStreamParameters?.maxRetransmits).toBeUndefined();
	expect(pipeDataConsumer.label).toBe('foo');
	expect(pipeDataConsumer.protocol).toBe('bar');

	expect(pipeDataProducer.id).toBe(dataProducer.id);
	expect(pipeDataProducer.closed).toBe(false);
	expect(pipeDataProducer.type).toBe('sctp');
	expect(typeof pipeDataProducer.sctpStreamParameters).toBe('object');
	expect(typeof pipeDataProducer.sctpStreamParameters?.streamId).toBe('number');
	expect(pipeDataProducer.sctpStreamParameters?.ordered).toBe(false);
	expect(pipeDataProducer.sctpStreamParameters?.maxPacketLifeTime).toBe(5000);
	expect(pipeDataProducer.sctpStreamParameters?.maxRetransmits).toBeUndefined();
	expect(pipeDataProducer.label).toBe('foo');
	expect(pipeDataProducer.protocol).toBe('bar');
}, 2000);

test('transport.dataConsume() for a pipe DataProducer succeeds', async () =>
{
	dataConsumer = await transport2.consumeData(
		{
			dataProducerId : dataProducer.id
		});

	expect(typeof dataConsumer.id).toBe('string');
	expect(dataConsumer.closed).toBe(false);
	expect(dataConsumer.type).toBe('sctp');
	expect(typeof dataConsumer.sctpStreamParameters).toBe('object');
	expect(typeof dataConsumer.sctpStreamParameters?.streamId).toBe('number');
	expect(dataConsumer.sctpStreamParameters?.ordered).toBe(false);
	expect(dataConsumer.sctpStreamParameters?.maxPacketLifeTime).toBe(5000);
	expect(dataConsumer.sctpStreamParameters?.maxRetransmits).toBeUndefined();
	expect(dataConsumer.label).toBe('foo');
	expect(dataConsumer.protocol).toBe('bar');
}, 2000);

test('dataProducer.close() is transmitted to pipe DataConsumer', async () =>
{
	await dataProducer.close();

	expect(dataProducer.closed).toBe(true);

	if (!dataConsumer.closed)
	{
		await new Promise<void>((resolve) => dataConsumer.once('dataproducerclose', resolve));
	}

	expect(dataConsumer.closed).toBe(true);
}, 2000);

test('router.pipeToRouter() called twice generates a single PipeTransport pair', async () =>
{
	const routerA = await worker1.createRouter({ mediaCodecs });
	const routerB = await worker2.createRouter({ mediaCodecs });
	const transportA1 = await routerA.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] });
	const transportA2 = await routerA.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] });
	const audioProducerA1 = await transportA1.produce(audioProducerParameters);
	const audioProducerA2 = await transportA2.produce(audioProducerParameters);
	let dump;

	await Promise.all(
		[
			routerA.pipeToRouter(
				{
					producerId : audioProducerA1.id,
					router     : routerB
				}),
			routerA.pipeToRouter(
				{
					producerId : audioProducerA2.id,
					router     : routerB
				})
		]);

	dump = await routerA.dump();

	// There should be 3 Transports in routerA:
	// - WebRtcTransport for audioProducerA1 and audioProducerA2.
	// - PipeTransport between routerA and routerB.
	expect(dump.transportIds.length).toBe(3);

	dump = await routerB.dump();

	// There should be 1 Transport in routerB:
	// - PipeTransport between routerA and routerB.
	expect(dump.transportIds.length).toBe(1);

	routerA.close();
	routerB.close();
}, 2000);

test('router.pipeToRouter() called in two Routers passing one to each other as argument generates a single a single PipeTransport pair', async () =>
{
	// We must close other Routers so previously generated PipeTransports are
	// closed and removed from the map.
	router1.close();
	router2.close();

	const routerA = await worker1.createRouter({ mediaCodecs });
	const routerB = await worker2.createRouter({ mediaCodecs });
	const transportA = await routerA.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] });
	const transportB = await routerB.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] });
	const audioProducerA = await transportA.produce(audioProducerParameters);
	const audioProducerB = await transportB.produce(audioProducerParameters);
	const pipeTransportsA = new Map();
	const pipeTransportsB = new Map();

	routerA.observer.on('newtransport', (transport) =>
	{
		if (transport.constructor.name !== 'PipeTransport')
		{
			return;
		}

		pipeTransportsA.set(transport.id, transport);

		transport.observer.on('close', () => pipeTransportsA.delete(transport.id));
	});

	routerB.observer.on('newtransport', (transport) =>
	{
		if (transport.constructor.name !== 'PipeTransport')
		{
			return;
		}

		pipeTransportsB.set(transport.id, transport);

		transport.observer.on('close', () => pipeTransportsB.delete(transport.id));
	});

	await Promise.all(
		[
			routerA.pipeToRouter(
				{
					producerId : audioProducerA.id,
					router     : routerB
				}),
			routerB.pipeToRouter(
				{
					producerId : audioProducerB.id,
					router     : routerA
				})
		]);

	// There should be a single PipeTransport in each Router and they must be
	// connected.

	expect(pipeTransportsA.size).toBe(1);
	expect(pipeTransportsB.size).toBe(1);

	const pipeTransportA = Array.from(pipeTransportsA.values())[0];
	const pipeTransportB = Array.from(pipeTransportsB.values())[0];

	expect(pipeTransportA.tuple.localPort).toBe(pipeTransportB.tuple.remotePort);
	expect(pipeTransportB.tuple.localPort).toBe(pipeTransportA.tuple.remotePort);

	routerA.close();

	expect(pipeTransportsA.size).toBe(0);
	expect(pipeTransportsB.size).toBe(0);

	routerB.close();
}, 2000);
