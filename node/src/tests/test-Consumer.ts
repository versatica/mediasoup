import * as flatbuffers from 'flatbuffers';
import * as mediasoup from '../';
import { UnsupportedError } from '../errors';
import { Notification, Body as NotificationBody, Event } from '../fbs/notification';
import * as FbsConsumer from '../fbs/consumer';

let worker: mediasoup.types.Worker;
let router: mediasoup.types.Router;
let transport1: mediasoup.types.WebRtcTransport;
let transport2: mediasoup.types.WebRtcTransport;
let audioProducer: mediasoup.types.Producer;
let videoProducer: mediasoup.types.Producer;
let audioConsumer: mediasoup.types.Consumer;
let videoConsumer: mediasoup.types.Consumer;
let videoPipeConsumer: mediasoup.types.Consumer;

const mediaCodecs: mediasoup.types.RtpCodecCapability[] =
[
	{
		kind       : 'audio',
		mimeType   : 'audio/opus',
		clockRate  : 48000,
		channels   : 2,
		parameters :
		{
			foo : 'bar'
		}
	},
	{
		kind      : 'video',
		mimeType  : 'video/VP8',
		clockRate : 90000
	},
	{
		kind       : 'video',
		mimeType   : 'video/H264',
		clockRate  : 90000,
		parameters :
		{
			'level-asymmetry-allowed' : 1,
			'packetization-mode'      : 1,
			'profile-level-id'        : '4d0032',
			foo                       : 'bar'
		}
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
					usedtx       : 1,
					foo          : 222.222,
					bar          : '333'
				}
			}
		],
		headerExtensions :
		[
			{
				uri : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id  : 10
			},
			{
				uri : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				id  : 12
			}
		],
		encodings : [ { ssrc: 11111111 } ],
		rtcp      :
		{
			cname : 'FOOBAR'
		}
	},
	appData : { foo: 1, bar: '2' }
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
				mimeType    : 'video/h264',
				payloadType : 112,
				clockRate   : 90000,
				parameters  :
				{
					'packetization-mode' : 1,
					'profile-level-id'   : '4d0032'
				},
				rtcpFeedback :
				[
					{ type: 'nack', parameter: '' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'goog-remb', parameter: '' }
				]
			},
			{
				mimeType    : 'video/rtx',
				payloadType : 113,
				clockRate   : 90000,
				parameters  : { apt: 112 }
			}
		],
		headerExtensions :
		[
			{
				uri : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id  : 10
			},
			{
				uri : 'urn:3gpp:video-orientation',
				id  : 13
			}
		],
		encodings :
		[
			{ ssrc: 22222222, rtx: { ssrc: 22222223 } },
			{ ssrc: 22222224, rtx: { ssrc: 22222225 } },
			{ ssrc: 22222226, rtx: { ssrc: 22222227 } },
			{ ssrc: 22222228, rtx: { ssrc: 22222229 } }
		],
		rtcp :
		{
			cname : 'FOOBAR'
		}
	},
	appData : { foo: 1, bar: '2' }
};

const consumerDeviceCapabilities: mediasoup.types.RtpCapabilities =
{
	codecs :
	[
		{
			mimeType             : 'audio/opus',
			kind                 : 'audio',
			preferredPayloadType : 100,
			clockRate            : 48000,
			channels             : 2,
			rtcpFeedback         :
			[
				{ type: 'nack', parameter: '' }
			]
		},
		{
			mimeType             : 'video/H264',
			kind                 : 'video',
			preferredPayloadType : 101,
			clockRate            : 90000,
			parameters           :
			{
				'level-asymmetry-allowed' : 1,
				'packetization-mode'      : 1,
				'profile-level-id'        : '4d0032'
			},
			rtcpFeedback :
			[
				{ type: 'nack', parameter: '' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb', parameter: '' }
			]
		},
		{
			mimeType             : 'video/rtx',
			kind                 : 'video',
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
			kind             : 'audio',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:mid',
			preferredId      : 1,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:mid',
			preferredId      : 1,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id',
			preferredId      : 2,
			preferredEncrypt : false
		},
		{
			kind             : 'audio',
			uri              : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time', // eslint-disable-line max-len
			preferredId      : 4,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
			uri              : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time', // eslint-disable-line max-len
			preferredId      : 4,
			preferredEncrypt : false
		},
		{
			kind             : 'audio',
			uri              : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
			preferredId      : 10,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
			uri              : 'urn:3gpp:video-orientation',
			preferredId      : 11,
			preferredEncrypt : false
		},
		{
			kind             : 'video',
			uri              : 'urn:ietf:params:rtp-hdrext:toffset',
			preferredId      : 12,
			preferredEncrypt : false
		}
	]
};

beforeAll(async () =>
{
	worker = await mediasoup.createWorker();
	router = await worker.createRouter({ mediaCodecs });
	transport1 = await router.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ]
		});
	transport2 = await router.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ]
		});
	audioProducer = await transport1.produce(audioProducerParameters);
	videoProducer = await transport1.produce(videoProducerParameters);

	// Pause the videoProducer.
	await videoProducer.pause();
});

afterAll(() => worker.close());

test('transport.consume() succeeds', async () =>
{
	const onObserverNewConsumer1 = jest.fn();

	transport2.observer.once('newconsumer', onObserverNewConsumer1);

	expect(router.canConsume(
		{
			producerId      : audioProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		}))
		.toBe(true);

	audioConsumer = await transport2.consume(
		{
			producerId      : audioProducer.id,
			rtpCapabilities : consumerDeviceCapabilities,
			appData         : { baz: 'LOL' }
		});

	expect(onObserverNewConsumer1).toHaveBeenCalledTimes(1);
	expect(onObserverNewConsumer1).toHaveBeenCalledWith(audioConsumer);
	expect(typeof audioConsumer.id).toBe('string');
	expect(audioConsumer.producerId).toBe(audioProducer.id);
	expect(audioConsumer.closed).toBe(false);
	expect(audioConsumer.kind).toBe('audio');
	expect(typeof audioConsumer.rtpParameters).toBe('object');
	expect(audioConsumer.rtpParameters.mid).toBe('0');
	expect(audioConsumer.rtpParameters.codecs.length).toBe(1);
	expect(audioConsumer.rtpParameters.codecs[0]).toEqual(
		{
			mimeType    : 'audio/opus',
			payloadType : 100,
			clockRate   : 48000,
			channels    : 2,
			parameters  :
			{
				useinbandfec : 1,
				usedtx       : 1,
				foo          : 222.222,
				bar          : '333'
			},
			rtcpFeedback : []
		});
	expect(audioConsumer.type).toBe('simple');
	expect(audioConsumer.paused).toBe(false);
	expect(audioConsumer.producerPaused).toBe(false);
	expect(audioConsumer.priority).toBe(1);
	expect(audioConsumer.score).toEqual(
		{ score: 10, producerScore: 0, producerScores: [ 0 ] });
	expect(audioConsumer.preferredLayers).toBeUndefined();
	expect(audioConsumer.currentLayers).toBeUndefined();
	expect(audioConsumer.appData).toEqual({ baz: 'LOL' });

	let dump = await router.dump();

	expect(dump.mapProducerIdConsumerIds)
		.toEqual(expect.arrayContaining([
			{ key: audioProducer.id, values: [ audioConsumer.id ] }
		]));

	expect(dump.mapConsumerIdProducerId)
		.toEqual(expect.arrayContaining([
			{ key: audioConsumer.id, value: audioProducer.id }
		]));

	await expect(transport2.dump())
		.resolves
		.toMatchObject(
			{
				id          : transport2.id,
				producerIds : [],
				consumerIds : [ audioConsumer.id ]
			});

	const onObserverNewConsumer2 = jest.fn();

	transport2.observer.once('newconsumer', onObserverNewConsumer2);

	expect(router.canConsume(
		{
			producerId      : videoProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		}))
		.toBe(true);

	videoConsumer = await transport2.consume(
		{
			producerId      : videoProducer.id,
			rtpCapabilities : consumerDeviceCapabilities,
			paused          : true,
			preferredLayers : { spatialLayer: 12 },
			appData         : { baz: 'LOL' }
		});

	expect(onObserverNewConsumer2).toHaveBeenCalledTimes(1);
	expect(onObserverNewConsumer2).toHaveBeenCalledWith(videoConsumer);
	expect(typeof videoConsumer.id).toBe('string');
	expect(videoConsumer.producerId).toBe(videoProducer.id);
	expect(videoConsumer.closed).toBe(false);
	expect(videoConsumer.kind).toBe('video');
	expect(typeof videoConsumer.rtpParameters).toBe('object');
	expect(videoConsumer.rtpParameters.mid).toBe('1');
	expect(videoConsumer.rtpParameters.codecs.length).toBe(2);
	expect(videoConsumer.rtpParameters.codecs[0]).toEqual(
		{
			mimeType    : 'video/H264',
			payloadType : 103,
			clockRate   : 90000,
			parameters  :
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '4d0032'
			},
			rtcpFeedback :
			[
				{ type: 'nack', parameter: '' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb', parameter: '' }
			]
		});
	expect(videoConsumer.rtpParameters.codecs[1]).toEqual(
		{
			mimeType     : 'video/rtx',
			payloadType  : 104,
			clockRate    : 90000,
			parameters   : { apt: 103 },
			rtcpFeedback : []
		});
	expect(videoConsumer.type).toBe('simulcast');
	expect(videoConsumer.paused).toBe(true);
	expect(videoConsumer.producerPaused).toBe(true);
	expect(videoConsumer.priority).toBe(1);
	expect(videoConsumer.score).toEqual(
		{ score: 10, producerScore: 0, producerScores: [ 0, 0, 0, 0 ] });
	expect(videoConsumer.preferredLayers).toEqual({ spatialLayer: 3, temporalLayer: 0 });
	expect(videoConsumer.currentLayers).toBeUndefined();
	expect(videoConsumer.appData).toEqual({ baz: 'LOL' });

	const onObserverNewConsumer3 = jest.fn();

	transport2.observer.once('newconsumer', onObserverNewConsumer3);

	expect(router.canConsume(
		{
			producerId      : videoProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		}))
		.toBe(true);

	videoPipeConsumer = await transport2.consume(
		{
			producerId      : videoProducer.id,
			rtpCapabilities : consumerDeviceCapabilities,
			pipe            : true
		});

	expect(onObserverNewConsumer3).toHaveBeenCalledTimes(1);
	expect(onObserverNewConsumer3).toHaveBeenCalledWith(videoPipeConsumer);
	expect(typeof videoPipeConsumer.id).toBe('string');
	expect(videoPipeConsumer.producerId).toBe(videoProducer.id);
	expect(videoPipeConsumer.closed).toBe(false);
	expect(videoPipeConsumer.kind).toBe('video');
	expect(typeof videoPipeConsumer.rtpParameters).toBe('object');
	expect(videoPipeConsumer.rtpParameters.mid).toBeUndefined();
	expect(videoPipeConsumer.rtpParameters.codecs.length).toBe(2);
	expect(videoPipeConsumer.rtpParameters.codecs[0]).toEqual(
		{
			mimeType    : 'video/H264',
			payloadType : 103,
			clockRate   : 90000,
			parameters  :
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '4d0032'
			},
			rtcpFeedback :
			[
				{ type: 'nack', parameter: '' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb', parameter: '' }
			]
		});
	expect(videoPipeConsumer.rtpParameters.codecs[1]).toEqual(
		{
			mimeType     : 'video/rtx',
			payloadType  : 104,
			clockRate    : 90000,
			parameters   : { apt: 103 },
			rtcpFeedback : []
		});
	expect(videoPipeConsumer.type).toBe('pipe');
	expect(videoPipeConsumer.paused).toBe(false);
	expect(videoPipeConsumer.producerPaused).toBe(true);
	expect(videoPipeConsumer.priority).toBe(1);
	expect(videoPipeConsumer.score).toEqual(
		{ score: 10, producerScore: 10, producerScores: [ 0, 0, 0, 0 ] });
	expect(videoPipeConsumer.preferredLayers).toBeUndefined();
	expect(videoPipeConsumer.currentLayers).toBeUndefined();
	expect(videoPipeConsumer.appData).toBeUndefined;

	dump = await router.dump();

	// Sort values for mapProducerIdConsumerIds.
	expect(Array.isArray(dump.mapProducerIdConsumerIds)).toBe(true);
	dump.mapProducerIdConsumerIds.forEach((entry: any) =>
	{
		entry.values = entry.values.sort();
	});

	expect(dump.mapProducerIdConsumerIds)
		.toEqual(expect.arrayContaining([
			{ key: audioProducer.id, values: [ audioConsumer.id ] }
		]));
	expect(dump.mapProducerIdConsumerIds)
		.toEqual(expect.arrayContaining([
			{ key: videoProducer.id, values: [ videoConsumer.id, videoPipeConsumer.id ].sort() }
		]));

	expect(dump.mapConsumerIdProducerId)
		.toEqual(expect.arrayContaining([
			{ key: audioConsumer.id, value: audioProducer.id }
		]));
	expect(dump.mapConsumerIdProducerId)
		.toEqual(expect.arrayContaining([
			{ key: videoConsumer.id, value: videoProducer.id }
		]));
	expect(dump.mapConsumerIdProducerId)
		.toEqual(expect.arrayContaining([
			{ key: videoPipeConsumer.id, value: videoProducer.id }
		]));

	await expect(transport2.dump())
		.resolves
		.toMatchObject(
			{
				id          : transport2.id,
				producerIds : [],
				consumerIds : expect.arrayContaining(
					[
						audioConsumer.id,
						videoConsumer.id,
						videoPipeConsumer.id
					])
			});
}, 2000);

test('transport.consume() with enableRtx succeeds', async () =>
{
	const audioConsumer2 = await transport2.consume(
		{
			producerId      : audioProducer.id,
			rtpCapabilities : consumerDeviceCapabilities,
			enableRtx       : true
		});

	expect(audioConsumer2.kind).toBe('audio');
	expect(audioConsumer2.rtpParameters.codecs.length).toBe(1);
	expect(audioConsumer2.rtpParameters.codecs[0]).toEqual(
		{
			mimeType    : 'audio/opus',
			payloadType : 100,
			clockRate   : 48000,
			channels    : 2,
			parameters  :
			{
				useinbandfec : 1,
				usedtx       : 1,
				foo          : 222.222,
				bar          : '333'
			},
			rtcpFeedback : [ { type: 'nack', parameter: '' } ]
		});

	audioConsumer2.close();
}, 2000);

test('transport.consume() can be created with user provided mid', async () =>
{
	const audioConsumer1 = await transport2.consume(
		{
			producerId      : audioProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		});

	expect(audioConsumer1.rtpParameters.mid).toEqual(
		expect.stringMatching(/^[0-9]+/));

	const audioConsumer2 = await transport2.consume(
		{
			producerId      : audioProducer.id,
			mid             : 'custom-mid',
			rtpCapabilities : consumerDeviceCapabilities
		});

	expect(audioConsumer2.rtpParameters.mid).toBe('custom-mid');

	const audioConsumer3 = await transport2.consume(
		{
			producerId      : audioProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		});

	expect(audioConsumer3.rtpParameters.mid).toEqual(
		expect.stringMatching(/^[0-9]+/));
	expect(Number(audioConsumer1.rtpParameters.mid) + 1).toBe(
		Number(audioConsumer3.rtpParameters.mid));

	audioConsumer3.close();
	audioConsumer2.close();
	audioConsumer1.close();
}, 2000);

test('transport.consume() with incompatible rtpCapabilities rejects with UnsupportedError', async () =>
{
	let invalidDeviceCapabilities: mediasoup.types.RtpCapabilities;

	invalidDeviceCapabilities =
	{
		codecs :
		[
			{
				kind                 : 'audio',
				mimeType             : 'audio/ISAC',
				preferredPayloadType : 100,
				clockRate            : 32000,
				channels             : 1
			}
		],
		headerExtensions : []
	};

	expect(router.canConsume(
		{ producerId: audioProducer.id, rtpCapabilities: invalidDeviceCapabilities }))
		.toBe(false);

	await expect(transport2.consume(
		{
			producerId      : audioProducer.id,
			rtpCapabilities : invalidDeviceCapabilities
		}))
		.rejects
		.toThrow(UnsupportedError);

	invalidDeviceCapabilities =
	{
		codecs           : [],
		headerExtensions : []
	};

	expect(router.canConsume(
		{ producerId: audioProducer.id, rtpCapabilities: invalidDeviceCapabilities }))
		.toBe(false);

	await expect(transport2.consume(
		{
			producerId      : audioProducer.id,
			rtpCapabilities : invalidDeviceCapabilities
		}))
		.rejects
		.toThrow(UnsupportedError);
}, 2000);

test('consumer.dump() succeeds', async () =>
{
	let data;

	data = await audioConsumer.dump();

	expect(data.id).toBe(audioConsumer.id);
	expect(data.producerId).toBe(audioConsumer.producerId);
	expect(data.kind).toBe(audioConsumer.kind);
	expect(typeof data.rtpParameters).toBe('object');
	expect(Array.isArray(data.rtpParameters.codecs)).toBe(true);
	expect(data.rtpParameters.codecs.length).toBe(1);
	expect(data.rtpParameters.codecs[0].mimeType).toBe('audio/opus');
	expect(data.rtpParameters.codecs[0].payloadType).toBe(100);
	expect(data.rtpParameters.codecs[0].clockRate).toBe(48000);
	expect(data.rtpParameters.codecs[0].channels).toBe(2);
	expect(data.rtpParameters.codecs[0].parameters)
		.toEqual(
			{
				useinbandfec : 1,
				usedtx       : 1,
				foo          : 222.222,
				bar          : '333'
			});
	expect(data.rtpParameters.codecs[0].rtcpFeedback).toEqual([]);
	expect(Array.isArray(data.rtpParameters.headerExtensions)).toBe(true);
	expect(data.rtpParameters.headerExtensions!.length).toBe(3);
	expect(data.rtpParameters.headerExtensions).toEqual(
		[
			{
				uri        : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id         : 1,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time',
				id         : 4,
				parameters : {},
				encrypt    : false
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				id         : 10,
				parameters : {},
				encrypt    : false
			}
		]);
	expect(Array.isArray(data.rtpParameters.encodings)).toBe(true);
	expect(data.rtpParameters.encodings!.length).toBe(1);
	expect(data.rtpParameters.encodings).toEqual(
		[
			expect.objectContaining(
				{
					codecPayloadType : 100,
					ssrc             : audioConsumer.rtpParameters.encodings?.[0].ssrc
				})
		]);
	expect(data.type).toBe('simple');
	expect(Array.isArray(data.consumableRtpEncodings)).toBe(true);
	expect(data.consumableRtpEncodings!.length).toBe(1);
	expect(data.consumableRtpEncodings).toEqual(
		[
			expect.objectContaining(
				{ ssrc: audioProducer.consumableRtpParameters.encodings?.[0].ssrc })
		]);
	expect(data.supportedCodecPayloadTypes).toEqual([ 100 ]);
	expect(data.paused).toBe(false);
	expect(data.producerPaused).toBe(false);
	expect(data.priority).toBe(1);

	data = await videoConsumer.dump();

	expect(data.id).toBe(videoConsumer.id);
	expect(data.producerId).toBe(videoConsumer.producerId);
	expect(data.kind).toBe(videoConsumer.kind);
	expect(typeof data.rtpParameters).toBe('object');
	expect(Array.isArray(data.rtpParameters.codecs)).toBe(true);
	expect(data.rtpParameters.codecs.length).toBe(2);
	expect(data.rtpParameters.codecs[0].mimeType).toBe('video/H264');
	expect(data.rtpParameters.codecs[0].payloadType).toBe(103);
	expect(data.rtpParameters.codecs[0].clockRate).toBe(90000);
	expect(data.rtpParameters.codecs[0].channels).toBeUndefined();
	expect(data.rtpParameters.codecs[0].parameters)
		.toEqual(
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '4d0032'
			});
	expect(data.rtpParameters.codecs[0].rtcpFeedback).toEqual(
		[
			{ type: 'nack' },
			{ type: 'nack', parameter: 'pli' },
			{ type: 'ccm', parameter: 'fir' },
			{ type: 'goog-remb' }
		]);
	expect(Array.isArray(data.rtpParameters.headerExtensions)).toBe(true);
	expect(data.rtpParameters.headerExtensions!.length).toBe(4);
	expect(data.rtpParameters.headerExtensions).toEqual(
		[
			{
				uri        : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id         : 1,
				encrypt    : false,
				parameters : {}
			},
			{
				uri        : 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time',
				id         : 4,
				parameters : {},
				encrypt    : false
			},
			{
				uri        : 'urn:3gpp:video-orientation',
				id         : 11,
				parameters : {},
				encrypt    : false
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:toffset',
				id         : 12,
				parameters : {},
				encrypt    : false
			}
		]);
	expect(Array.isArray(data.rtpParameters.encodings)).toBe(true);
	expect(data.rtpParameters.encodings!.length).toBe(1);
	expect(data.rtpParameters.encodings).toMatchObject(
		[
			{
				codecPayloadType : 103,
				ssrc             : videoConsumer.rtpParameters.encodings?.[0].ssrc,
				rtx              :
				{
					ssrc : videoConsumer.rtpParameters.encodings?.[0].rtx?.ssrc
				},
				scalabilityMode : 'L4T1'
			}
		]);
	expect(Array.isArray(data.consumableRtpEncodings)).toBe(true);
	expect(data.consumableRtpEncodings!.length).toBe(4);
	expect(data.consumableRtpEncodings![0]).toEqual(
		expect.objectContaining(
			{ ssrc: videoProducer.consumableRtpParameters.encodings?.[0].ssrc })
	);
	expect(data.consumableRtpEncodings![1]).toEqual(
		expect.objectContaining(
			{ ssrc: videoProducer.consumableRtpParameters.encodings?.[1].ssrc })
	);
	expect(data.consumableRtpEncodings![2]).toEqual(
		expect.objectContaining(
			{ ssrc: videoProducer.consumableRtpParameters.encodings?.[2].ssrc })
	);
	expect(data.consumableRtpEncodings![3]).toEqual(
		expect.objectContaining(
			{ ssrc: videoProducer.consumableRtpParameters.encodings?.[3].ssrc })
	);
	expect(data.supportedCodecPayloadTypes).toEqual([ 103 ]);
	expect(data.paused).toBe(true);
	expect(data.producerPaused).toBe(true);
	expect(data.priority).toBe(1);
}, 2000);

test('consumer.getStats() succeeds', async () =>
{
	await expect(audioConsumer.getStats())
		.resolves
		.toEqual(
			[
				expect.objectContaining(
					{
						type     : 'outbound-rtp',
						kind     : 'audio',
						mimeType : 'audio/opus',
						ssrc     : audioConsumer.rtpParameters.encodings?.[0].ssrc
					})
			]);

	await expect(videoConsumer.getStats())
		.resolves
		.toEqual(
			[
				expect.objectContaining(
					{
						type     : 'outbound-rtp',
						kind     : 'video',
						mimeType : 'video/H264',
						ssrc     : videoConsumer.rtpParameters.encodings?.[0].ssrc
					})
			]);
}, 2000);

test('consumer.pause() and resume() succeed', async () =>
{
	const onObserverPause = jest.fn();
	const onObserverResume = jest.fn();

	audioConsumer.observer.on('pause', onObserverPause);
	audioConsumer.observer.on('resume', onObserverResume);

	await audioConsumer.pause();
	expect(audioConsumer.paused).toBe(true);

	await expect(audioConsumer.dump())
		.resolves
		.toMatchObject({ paused: true });

	await audioConsumer.resume();
	expect(audioConsumer.paused).toBe(false);

	await expect(audioConsumer.dump())
		.resolves
		.toMatchObject({ paused: false });

	// Even if we don't await for pause()/resume() completion, the observer must
	// fire 'pause' and 'resume' events if state was the opposite.
	audioConsumer.pause();
	audioConsumer.resume();
	audioConsumer.pause();
	audioConsumer.pause();
	audioConsumer.pause();
	await audioConsumer.resume();

	expect(onObserverPause).toHaveBeenCalledTimes(3);
	expect(onObserverResume).toHaveBeenCalledTimes(3);
}, 2000);

test('producer.pause() and resume() emit events', async () =>
{
	const promises = [];
	const events: string[] = [];
	
	audioConsumer.observer.once('resume', () => 
	{
		events.push('resume');
	});

	audioConsumer.observer.once('pause', () => 
	{
		events.push('pause');
	});

	promises.push(audioProducer.pause());
	promises.push(audioProducer.resume());

	await Promise.all(promises);

	// Must also wait a bit for the corresponding events in the consumer.
	await new Promise((resolve) => setTimeout(resolve, 100));
	
	expect(events).toEqual([ 'pause', 'resume' ]);
	expect(audioConsumer.paused).toBe(false);
}, 2000);

test('consumer.setPreferredLayers() succeed', async () =>
{
	await audioConsumer.setPreferredLayers({ spatialLayer: 1, temporalLayer: 1 });
	expect(audioConsumer.preferredLayers).toBeUndefined();

	await videoConsumer.setPreferredLayers({ spatialLayer: 2, temporalLayer: 3 });
	expect(videoConsumer.preferredLayers).toEqual({ spatialLayer: 2, temporalLayer: 0 });
}, 2000);

test('consumer.setPreferredLayers() with wrong arguments rejects with TypeError', async () =>
{
	// @ts-ignore
	await expect(videoConsumer.setPreferredLayers({}))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(videoConsumer.setPreferredLayers({ foo: '123' }))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(videoConsumer.setPreferredLayers('foo'))
		.rejects
		.toThrow(TypeError);

	// Missing spatialLayer.
	// @ts-ignore
	await expect(videoConsumer.setPreferredLayers({ temporalLayer: 2 }))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('consumer.setPriority() succeed', async () =>
{
	await videoConsumer.setPriority(2);
	expect(videoConsumer.priority).toBe(2);
}, 2000);

test('consumer.setPriority() with wrong arguments rejects with TypeError', async () =>
{
	// @ts-ignore
	await expect(videoConsumer.setPriority())
		.rejects
		.toThrow(TypeError);

	await expect(videoConsumer.setPriority(0))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(videoConsumer.setPriority('foo'))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('consumer.unsetPriority() succeed', async () =>
{
	await videoConsumer.unsetPriority();
	expect(videoConsumer.priority).toBe(1);
}, 2000);

test('consumer.enableTraceEvent() succeed', async () =>
{
	let dump;

	await audioConsumer.enableTraceEvent([ 'rtp', 'pli' ]);
	dump = await audioConsumer.dump();
	expect(dump.traceEventTypes)
		.toEqual(expect.arrayContaining([ 'rtp', 'pli' ]));

	await audioConsumer.enableTraceEvent([]);
	dump = await audioConsumer.dump();
	expect(dump.traceEventTypes)
		.toEqual(expect.arrayContaining([]));

	// @ts-ignore
	await audioConsumer.enableTraceEvent([ 'nack', 'FOO', 'fir' ]);
	dump = await audioConsumer.dump();
	expect(dump.traceEventTypes)
		.toEqual(expect.arrayContaining([ 'nack', 'fir' ]));

	await audioConsumer.enableTraceEvent();
	dump = await audioConsumer.dump();
	expect(dump.traceEventTypes)
		.toEqual(expect.arrayContaining([]));
}, 2000);

test('consumer.enableTraceEvent() with wrong arguments rejects with TypeError', async () =>
{
	// @ts-ignore
	await expect(audioConsumer.enableTraceEvent(123))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(audioConsumer.enableTraceEvent('rtp'))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(audioConsumer.enableTraceEvent([ 'fir', 123.123 ]))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('Consumer emits "producerpause" and "producerresume"', async () =>
{
	await new Promise<void>((resolve) =>
	{
		audioConsumer.on('producerpause', resolve);
		audioProducer.pause();
	});

	expect(audioConsumer.paused).toBe(false);
	expect(audioConsumer.producerPaused).toBe(true);

	await new Promise<void>((resolve) =>
	{
		audioConsumer.on('producerresume', resolve);
		audioProducer.resume();
	});

	expect(audioConsumer.paused).toBe(false);
	expect(audioConsumer.producerPaused).toBe(false);
}, 2000);

test('Consumer emits "score"', async () =>
{
	// Private API.
	const channel = audioConsumer.channelForTesting;
	const onScore = jest.fn();

	audioConsumer.on('score', onScore);

	// Simulate a 'score' notification coming through the channel.
	const builder = new flatbuffers.Builder();
	const consumerScore = new FbsConsumer.ConsumerScoreT(9, 10, [ 8 ]);
	const consumerScoreNotification = new FbsConsumer.ScoreNotificationT(consumerScore);
	const notificationOffset = Notification.createNotification(
		builder,
		builder.createString(audioConsumer.id),
		Event.CONSUMER_SCORE,
		NotificationBody.Consumer_ScoreNotification,
		consumerScoreNotification.pack(builder)
	);

	builder.finish(notificationOffset);

	const notification = Notification.getRootAsNotification(
		new flatbuffers.ByteBuffer(builder.asUint8Array()));

	channel.emit(audioConsumer.id, Event.CONSUMER_SCORE, notification);
	channel.emit(audioConsumer.id, Event.CONSUMER_SCORE, notification);
	channel.emit(audioConsumer.id, Event.CONSUMER_SCORE, notification);

	expect(onScore).toHaveBeenCalledTimes(3);
	expect(audioConsumer.score).toEqual(
		{ score: 9, producerScore: 10, producerScores: [ 8 ] });
}, 2000);

test('consumer.close() succeeds', async () =>
{
	const onObserverClose = jest.fn();

	audioConsumer.observer.once('close', onObserverClose);
	audioConsumer.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(audioConsumer.closed).toBe(true);

	const routerDump = await router.dump();

	expect(routerDump.mapProducerIdConsumerIds)
		.toEqual(expect.arrayContaining([
			{ key: audioProducer.id, values: [ ] }
		]));

	expect(routerDump.mapConsumerIdProducerId)
		.toEqual(expect.arrayContaining([
			{ key: videoConsumer.id, value: videoProducer.id }
		]));
	expect(routerDump.mapConsumerIdProducerId)
		.toEqual(expect.arrayContaining([
			{ key: videoPipeConsumer.id, value: videoProducer.id }
		]));

	const transportDump = await transport2.dump();

	transportDump.consumerIds = transportDump.consumerIds.sort();

	expect(transportDump)
		.toMatchObject(
			{
				id          : transport2.id,
				producerIds : [],
				consumerIds : [ videoConsumer.id, videoPipeConsumer.id ].sort()
			});
}, 2000);

test('Consumer methods reject if closed', async () =>
{
	await expect(audioConsumer.dump())
		.rejects
		.toThrow(Error);

	await expect(audioConsumer.getStats())
		.rejects
		.toThrow(Error);

	await expect(audioConsumer.pause())
		.rejects
		.toThrow(Error);

	await expect(audioConsumer.resume())
		.rejects
		.toThrow(Error);

	// @ts-ignore
	await expect(audioConsumer.setPreferredLayers({}))
		.rejects
		.toThrow(Error);

	await expect(audioConsumer.setPriority(2))
		.rejects
		.toThrow(Error);

	await expect(audioConsumer.requestKeyFrame())
		.rejects
		.toThrow(Error);
}, 2000);

test('Consumer emits "producerclose" if Producer is closed', async () =>
{
	audioConsumer = await transport2.consume(
		{
			producerId      : audioProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		});

	const onObserverClose = jest.fn();

	audioConsumer.observer.once('close', onObserverClose);

	await new Promise<void>((resolve) =>
	{
		audioConsumer.on('producerclose', resolve);
		audioProducer.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(audioConsumer.closed).toBe(true);
}, 2000);

test('Consumer emits "transportclose" if Transport is closed', async () =>
{
	videoConsumer = await transport2.consume(
		{
			producerId      : videoProducer.id,
			rtpCapabilities : consumerDeviceCapabilities
		});

	const onObserverClose = jest.fn();

	videoConsumer.observer.once('close', onObserverClose);

	await new Promise<void>((resolve) =>
	{
		videoConsumer.on('transportclose', resolve);
		transport2.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(videoConsumer.closed).toBe(true);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapProducerIdConsumerIds : {},
				mapConsumerIdProducerId  : {}
			});
}, 2000);
