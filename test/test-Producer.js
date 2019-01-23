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
							foo          : '222',
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
				encodings :
				[
					{ ssrc: 11111111 }
				],
				rtcp :
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
	// Private API.
	expect(audioProducer.consumableRtpParameters).toBeType('object');
	expect(audioProducer.paused).toBe(false);
	expect(audioProducer.appData).toEqual({ foo: 1, bar: '2' });

	expect(webRtcTransport.getProducerById(audioProducer.id)).toBe(audioProducer);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				id                       : router.id,
				mapProducerIdConsumerIds : { [audioProducer.id]: [] },
				mapConsumerIdProducerId  : {}
			});
}, 1000);

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
						uri : 'urn:3gpp:video-orientation',
						id  : 13
					}
				],
				encodings :
				[
					{ ssrc: 22222222, rtxSsrc: 22222223 }
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
	// Private API.
	expect(videoProducer.consumableRtpParameters).toBeType('object');
	expect(videoProducer.paused).toBe(false);
	expect(videoProducer.appData).toEqual({ foo: 1, bar: '2' });

	expect(plainRtpTransport.getProducerById(videoProducer.id)).toBe(videoProducer);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				id                       : router.id,
				mapProducerIdConsumerIds : { [videoProducer.id]: [] },
				mapConsumerIdProducerId  : {}
			});
}, 1000);

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
				rtcp             : { cname: 'audio' }
			}
		}))
		.rejects
		.toThrow(TypeError);

	// Missing or empty rtpParameters.encodings.
	await expect(webRtcTransport.produce(
		{
			kind          : 'audio',
			rtpParameters :
			{
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
				encodings        : [],
				rtcp             : { cname: 'audio' }
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
						name        : 'OPUS',
						mimeType    : 'audio/opus',
						payloadType : 111,
						clockRate   : 48000,
						channels    : 2
					}
				],
				headerExtensions : [],
				encodings        : [ { ssrc: 1111 } ]
			}
		}))
		.rejects
		.toThrow(TypeError);
}, 1000);

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
}, 1000);

test('producer.dump() succeeds', async () =>
{
	await expect(audioProducer.dump())
		.resolves
		.toMatchObject(
			{
				id             : audioProducer.id,
				kind           : 'audio',
				rtpStreams     : [],
				lossPercentage : 0,
				paused         : false
			});

	await expect(videoProducer.dump())
		.resolves
		.toMatchObject(
			{
				id             : videoProducer.id,
				kind           : 'video',
				rtpStreams     : [],
				lossPercentage : 0,
				paused         : false
			});
}, 1000);

test('producer.getStats() succeeds', async () =>
{
	await expect(audioProducer.getStats())
		.resolves
		.toEqual([]);

	await expect(videoProducer.getStats())
		.resolves
		.toEqual([]);
}, 1000);

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
}, 1000);

test('producer.close() succeeds', async () =>
{
	audioProducer.close();
	expect(audioProducer.closed).toBe(true);
	expect(webRtcTransport.getProducerById(audioProducer.id)).toBe(undefined);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				id                       : router.id,
				mapProducerIdConsumerIds : {},
				mapConsumerIdProducerId  : {}
			});
}, 1000);

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
}, 1000);

test('Producer emits "transportclose" if Transport is closed', async () =>
{
	await new Promise((resolve) =>
	{
		videoProducer.on('transportclose', resolve);

		plainRtpTransport.close();
	});

	expect(videoProducer.closed).toBe(true);
}, 1000);
