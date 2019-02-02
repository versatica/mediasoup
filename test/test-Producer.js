const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;
const { UnsupportedError } = require('../lib/errors');

expect.extend({ toBeType });

let worker;
let router;
let webRtcTransport;
let plainRtpTransport;
let audioProducer;
let videoProducer;

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
			useinbandfec : 0,
			foo          : '111'
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
			'level-asymmetry-allowed' : 1,
			'packetization-mode'      : 1,
			'profile-level-id'        : '4d0032',
			foo                       : 'bar'
		}
	}
];

beforeAll(async () =>
{
	worker = await createWorker();
	router = await worker.createRouter({ mediaCodecs });
	webRtcTransport = await router.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ]
		});
	plainRtpTransport = await router.createPlainRtpTransport(
		{
			listenIp : '127.0.0.1'
		});
});

afterAll(() => worker.close());

test('webRtcTransport.produce() succeeds', async () =>
{
	audioProducer = await webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters :
			{
				mid    : 'AUDIO',
				codecs :
				[
					{
						name        : 'OPUS',
						mimeType    : 'audio/opus',
						payloadType : 111,
						clockRate   : 48000,
						channels    : 2,
						parameters  :
						{
							useinbandfec : 1,
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
					cname : 'audio-1'
				}
			},
			appData : { foo: 1, bar: '2' }
		});

	expect(audioProducer.id).toBeType('string');
	expect(audioProducer.closed).toBe(false);
	expect(audioProducer.kind).toBe('audio');
	expect(audioProducer.rtpParameters).toBeType('object');
	expect(audioProducer.type).toBe('simple');
	// Private API.
	expect(audioProducer.consumableRtpParameters).toBeType('object');
	expect(audioProducer.paused).toBe(false);
	expect(audioProducer.score).toEqual([]);
	expect(audioProducer.appData).toEqual({ foo: 1, bar: '2' });

	expect(webRtcTransport.getProducerById(audioProducer.id)).toBe(audioProducer);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapProducerIdConsumerIds : { [audioProducer.id]: [] },
				mapConsumerIdProducerId  : {}
			});

	await expect(webRtcTransport.dump())
		.resolves
		.toMatchObject(
			{
				id          : webRtcTransport.id,
				producerIds : [ audioProducer.id ],
				consumerIds : []
			});
}, 2000);

test('plainRtpTransport.produce() succeeds', async () =>
{
	videoProducer = await plainRtpTransport.produce(
		{
			kind          : 'video',
			rtpParameters :
			{
				mid    : 'VIDEO',
				codecs :
				[
					{
						name        : 'H264',
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
							{ type: 'nack' },
							{ type: 'nack', parameter: 'pli' },
							{ type: 'goog-remb' }
						]
					},
					{
						name        : 'rtx',
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
					cname : 'video-1'
				}
			},
			appData : { foo: 1, bar: '2' }
		});

	expect(videoProducer.id).toBeType('string');
	expect(videoProducer.closed).toBe(false);
	expect(videoProducer.kind).toBe('video');
	expect(videoProducer.rtpParameters).toBeType('object');
	expect(videoProducer.type).toBe('simulcast');
	// Private API.
	expect(videoProducer.consumableRtpParameters).toBeType('object');
	expect(videoProducer.paused).toBe(false);
	expect(videoProducer.score).toEqual([]);
	expect(videoProducer.appData).toEqual({ foo: 1, bar: '2' });

	expect(plainRtpTransport.getProducerById(videoProducer.id)).toBe(videoProducer);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapProducerIdConsumerIds : { [videoProducer.id]: [] },
				mapConsumerIdProducerId  : {}
			});

	await expect(plainRtpTransport.dump())
		.resolves
		.toMatchObject(
			{
				id          : plainRtpTransport.id,
				producerIds : [ videoProducer.id ],
				consumerIds : []
			});
}, 2000);

test('webRtcTransport.produce() with wrong arguments rejects with TypeError', async () =>
{
	await expect(webRtcTransport.produce(
		{
			kind          : 'chicken',
			rtpParameters : {}
		}))
		.rejects
		.toThrow(TypeError);

	await expect(webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters : {}
		}))
		.rejects
		.toThrow(TypeError);

	// Missing or empty rtpParameters.codecs.
	await expect(webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters :
			{
				codecs           : [],
				headerExtensions : [],
				encodings        : [ { ssrc: '1111' } ],
				rtcp             : { cname: 'qwerty'	}
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Missing or empty rtpParameters.encodings.
	await expect(webRtcTransport.produce(
		{
			kind          : 'video',
			rtpParameters :
			{
				codecs :
				[
					{
						name        : 'H264',
						mimeType    : 'video/h264',
						payloadType : 112,
						clockRate   : 90000,
						parameters  :
						{
							'packetization-mode' : 1,
							'profile-level-id'   : '4d0032'
						}
					},
					{
						name        : 'rtx',
						mimeType    : 'video/rtx',
						payloadType : 113,
						clockRate   : 90000,
						parameters  : { apt: 112 }
					}
				],
				headerExtensions : [],
				encodings        : [],
				rtcp             : { cname: 'qwerty' }
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Missing rtpParameters.rtcp.
	await expect(webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters :
			{
				codecs :
				[
					{
						name        : 'H264',
						mimeType    : 'video/h264',
						payloadType : 112,
						clockRate   : 90000,
						parameters  :
						{
							'packetization-mode' : 1,
							'profile-level-id'   : '4d0032'
						}
					},
					{
						name        : 'rtx',
						mimeType    : 'video/rtx',
						payloadType : 113,
						clockRate   : 90000,
						parameters  : { apt: 112 }
					}
				],
				headerExtensions : [],
				encodings        :
				[
					{ ssrc: 6666, rtx: { ssrc: 6667 } }
				]
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Wrong apt in RTX codec.
	await expect(webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters :
			{
				codecs :
				[
					{
						name        : 'H264',
						mimeType    : 'video/h264',
						payloadType : 112,
						clockRate   : 90000,
						parameters  :
						{
							'packetization-mode' : 1,
							'profile-level-id'   : '4d0032'
						}
					},
					{
						name        : 'rtx',
						mimeType    : 'video/rtx',
						payloadType : 113,
						clockRate   : 90000,
						parameters  : { apt: 111 }
					}
				],
				headerExtensions : [],
				encodings        :
				[
					{ ssrc: 6666, rtx: { ssrc: 6667 } }
				],
				rtcp :
				{
					cname : 'video-1'
				}
			}
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('webRtcTransport.produce() with unsupported codecs rejects with UnsupportedError', async () =>
{
	// Missing or empty rtpParameters.encodings.
	await expect(webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters :
			{
				codecs :
				[
					{
						name        : 'ISAC',
						mimeType    : 'audio/ISAC',
						payloadType : 108,
						clockRate   : 32000
					}
				],
				headerExtensions : [],
				encodings        : [ { ssrc: 1111 } ],
				rtcp             : { cname: 'audio' }
			}
		}))
		.rejects
		.toThrow(UnsupportedError);
}, 2000);

test('webRtcTransport.produce() with already used MID or SSRC rejects with Error', async () =>
{
	await expect(webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters :
			{
				mid    : 'AUDIO',
				codecs :
				[
					{
						name        : 'OPUS',
						mimeType    : 'audio/opus',
						payloadType : 111,
						clockRate   : 48000,
						channels    : 2
					}
				],
				headerExtensions : [],
				encodings        : [ { ssrc: 33333333 } ],
				rtcp             :
				{
					cname : 'audio-2'
				}
			}
		}))
		.rejects
		.toThrow(Error);

	await expect(webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters :
			{
				mid    : 'AUDIO-2',
				codecs :
				[
					{
						name        : 'OPUS',
						mimeType    : 'audio/opus',
						payloadType : 111,
						clockRate   : 48000,
						channels    : 2
					}
				],
				headerExtensions : [],
				encodings        : [ { ssrc: 11111111 } ],
				rtcp             :
				{
					cname : 'audio-2'
				}
			}
		}))
		.rejects
		.toThrow(Error);
}, 2000);

test('producer.dump() succeeds', async () =>
{
	let data;

	data = await audioProducer.dump();

	expect(data.id).toBe(audioProducer.id);
	expect(data.kind).toBe(audioProducer.kind);
	expect(data.rtpParameters).toBeType('object');
	expect(data.rtpParameters.codecs).toBeType('array');
	expect(data.rtpParameters.codecs.length).toBe(1);
	expect(data.rtpParameters.codecs[0].name).toBe('opus');
	expect(data.rtpParameters.codecs[0].mimeType).toBe('audio/opus');
	expect(data.rtpParameters.codecs[0].payloadType).toBe(111);
	expect(data.rtpParameters.codecs[0].clockRate).toBe(48000);
	expect(data.rtpParameters.codecs[0].channels).toBe(2);
	expect(data.rtpParameters.codecs[0].parameters)
		.toEqual(
			{
				useinbandfec : 1,
				foo          : 222.222,
				bar          : '333'
			});
	expect(data.rtpParameters.codecs[0].rtcpFeedback).toEqual([]);
	expect(data.rtpParameters.headerExtensions).toBeType('array');
	expect(data.rtpParameters.headerExtensions.length).toBe(2);
	expect(data.rtpParameters.headerExtensions).toEqual(
		[
			{
				uri        : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id         : 10,
				parameters : {},
				encrypt    : false
			},
			{
				uri        : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				id         : 12,
				parameters : {},
				encrypt    : false
			}
		]);
	expect(data.rtpParameters.encodings).toBeType('array');
	expect(data.rtpParameters.encodings.length).toBe(1);
	expect(data.rtpParameters.encodings).toEqual(
		[
			{ codecPayloadType: 111, ssrc: 11111111 }
		]);
	expect(data.type).toBe('simple');

	data = await videoProducer.dump();

	expect(data.id).toBe(videoProducer.id);
	expect(data.kind).toBe(videoProducer.kind);
	expect(data.rtpParameters).toBeType('object');
	expect(data.rtpParameters.codecs).toBeType('array');
	expect(data.rtpParameters.codecs.length).toBe(2);
	expect(data.rtpParameters.codecs[0].name).toBe('H264');
	expect(data.rtpParameters.codecs[0].mimeType).toBe('video/H264');
	expect(data.rtpParameters.codecs[0].payloadType).toBe(112);
	expect(data.rtpParameters.codecs[0].clockRate).toBe(90000);
	expect(data.rtpParameters.codecs[0].channels).toBe(undefined);
	expect(data.rtpParameters.codecs[0].parameters)
		.toEqual(
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '4d0032'
			});
	expect(data.rtpParameters.codecs[0].rtcpFeedback)
		.toEqual(
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'goog-remb' }
			]);
	expect(data.rtpParameters.codecs[1].name).toBe('rtx');
	expect(data.rtpParameters.codecs[1].mimeType).toBe('video/rtx');
	expect(data.rtpParameters.codecs[1].payloadType).toBe(113);
	expect(data.rtpParameters.codecs[1].clockRate).toBe(90000);
	expect(data.rtpParameters.codecs[1].channels).toBe(undefined);
	expect(data.rtpParameters.codecs[1].parameters).toEqual({ apt: 112 });
	expect(data.rtpParameters.codecs[1].rtcpFeedback).toEqual([]);
	expect(data.rtpParameters.headerExtensions).toBeType('array');
	expect(data.rtpParameters.headerExtensions.length).toBe(2);
	expect(data.rtpParameters.headerExtensions).toEqual(
		[
			{
				uri        : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id         : 10,
				parameters : {},
				encrypt    : false
			},
			{
				uri        : 'urn:3gpp:video-orientation',
				id         : 13,
				parameters : {},
				encrypt    : false
			}
		]);
	expect(data.rtpParameters.encodings).toBeType('array');
	expect(data.rtpParameters.encodings.length).toBe(4);
	expect(data.rtpParameters.encodings).toEqual(
		[
			{ codecPayloadType: 112, ssrc: 22222222, rtx: { ssrc: 22222223 } },
			{ codecPayloadType: 112, ssrc: 22222224, rtx: { ssrc: 22222225 } },
			{ codecPayloadType: 112, ssrc: 22222226, rtx: { ssrc: 22222227 } },
			{ codecPayloadType: 112, ssrc: 22222228, rtx: { ssrc: 22222229 } }
		]);
	expect(data.type).toBe('simulcast');
}, 2000);

test('producer.getStats() succeeds', async () =>
{
	await expect(audioProducer.getStats())
		.resolves
		.toEqual([]);

	await expect(videoProducer.getStats())
		.resolves
		.toEqual([]);
}, 2000);

test('producer.pause() and resume() succeed', async () =>
{
	await audioProducer.pause();
	expect(audioProducer.paused).toBe(true);

	await expect(audioProducer.dump())
		.resolves
		.toMatchObject({ paused: true });

	await audioProducer.resume();
	expect(audioProducer.paused).toBe(false);

	await expect(audioProducer.dump())
		.resolves
		.toMatchObject({ paused: false });
}, 2000);

test('Producer emits "score"', async () =>
{
	// Private API.
	const channel = videoProducer._channel;

	const onScore = jest.fn();

	videoProducer.on('score', onScore);

	channel.emit(videoProducer.id, 'score', [ 10, 10, 9 ]);
	channel.emit(videoProducer.id, 'score', [ 10, 9, 8 ]);
	channel.emit(videoProducer.id, 'score', [ 10, 9, 7 ]);

	expect(onScore).toHaveBeenCalledTimes(3);
	expect(videoProducer.score).toEqual([ 10, 9, 7 ]);
}, 2000);

test('producer.close() succeeds', async () =>
{
	audioProducer.close();
	expect(audioProducer.closed).toBe(true);
	expect(webRtcTransport.getProducerById(audioProducer.id)).toBe(undefined);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapProducerIdConsumerIds : {},
				mapConsumerIdProducerId  : {}
			});

	await expect(webRtcTransport.dump())
		.resolves
		.toMatchObject(
			{
				id          : webRtcTransport.id,
				producerIds : [],
				consumerIds : []
			});
}, 2000);

test('Producer methods reject if closed', async () =>
{
	await expect(audioProducer.dump())
		.rejects
		.toThrow(Error);

	await expect(audioProducer.getStats())
		.rejects
		.toThrow(Error);

	await expect(audioProducer.pause())
		.rejects
		.toThrow(Error);

	await expect(audioProducer.resume())
		.rejects
		.toThrow(Error);
}, 2000);

test('Producer emits "transportclose" if Transport is closed', async () =>
{
	await new Promise((resolve) =>
	{
		videoProducer.on('transportclose', resolve);

		plainRtpTransport.close();
	});

	expect(videoProducer.closed).toBe(true);
}, 2000);
