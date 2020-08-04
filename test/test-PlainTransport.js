const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;

expect.extend({ toBeType });

let worker;
let router;
let transport;

const mediaCodecs =
[
	{
		kind       : 'audio',
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
		},
		rtcpFeedback : [] // Will be ignored.
	}
];

beforeAll(async () =>
{
	worker = await createWorker();
	router = await worker.createRouter({ mediaCodecs });
});

afterAll(() => worker.close());

beforeEach(async () =>
{
	transport = await router.createPlainTransport(
		{
			listenIp : { ip: '127.0.0.1', announcedIp: '4.4.4.4' },
			rtcpMux  : false
		});
});

afterEach(() => transport.close());

test('router.createPlainTransport() succeeds', async () =>
{
	await expect(router.dump())
		.resolves
		.toMatchObject({ transportIds: [ transport.id ] });

	const onObserverNewTransport = jest.fn();

	router.observer.once('newtransport', onObserverNewTransport);

	// Create a separate transport here.
	const transport1 = await router.createPlainTransport(
		{
			listenIp   : { ip: '127.0.0.1', announcedIp: '9.9.9.1' },
			rtcpMux    : true,
			enableSctp : true,
			appData    : { foo: 'bar' }
		});

	expect(onObserverNewTransport).toHaveBeenCalledTimes(1);
	expect(onObserverNewTransport).toHaveBeenCalledWith(transport1);
	expect(transport1.id).toBeType('string');
	expect(transport1.closed).toBe(false);
	expect(transport1.appData).toEqual({ foo: 'bar' });
	expect(transport1.tuple).toBeType('object');
	expect(transport1.tuple.localIp).toBe('9.9.9.1');
	expect(transport1.tuple.localPort).toBeType('number');
	expect(transport1.tuple.protocol).toBe('udp');
	expect(transport1.rtcpTuple).toBeUndefined();
	expect(transport1.sctpParameters).toStrictEqual(
		{
			port               : 5000,
			OS                 : 1024,
			MIS                : 1024,
			maxMessageSize     : 262144,
			isDataChannel      : false,
			sctpBufferedAmount : 0,
			sendBufferSize     : 262144
		});
	expect(transport1.sctpState).toBe('new');
	expect(transport1.srtpParameters).toBeUndefined();

	const data1 = await transport1.dump();

	expect(data1.id).toBe(transport1.id);
	expect(data1.direct).toBe(false);
	expect(data1.producerIds).toEqual([]);
	expect(data1.consumerIds).toEqual([]);
	expect(data1.tuple).toEqual(transport1.tuple);
	expect(data1.rtcpTuple).toEqual(transport1.rtcpTuple);
	expect(data1.sctpParameters).toEqual(transport1.sctpParameters);
	expect(data1.sctpState).toBe('new');
	expect(data1.recvRtpHeaderExtensions).toBeType('object');
	expect(data1.rtpListener).toBeType('object');

	transport1.close();
	expect(transport1.closed).toBe(true);

	await expect(router.createPlainTransport({ listenIp: '127.0.0.1' }))
		.resolves
		.toBeType('object');

	const transport2 = await router.createPlainTransport(
		{
			listenIp : '127.0.0.1',
			rtcpMux  : false
		});

	expect(transport2.id).toBeType('string');
	expect(transport2.closed).toBe(false);
	expect(transport2.appData).toEqual({});
	expect(transport2.tuple).toBeType('object');
	expect(transport2.tuple.localIp).toBe('127.0.0.1');
	expect(transport2.tuple.localPort).toBeType('number');
	expect(transport2.tuple.protocol).toBe('udp');
	expect(transport2.rtcpTuple).toBeType('object');
	expect(transport2.rtcpTuple.localIp).toBe('127.0.0.1');
	expect(transport2.rtcpTuple.localPort).toBeType('number');
	expect(transport2.rtcpTuple.protocol).toBe('udp');
	expect(transport2.sctpParameters).toBeUndefined();
	expect(transport2.sctpState).toBeUndefined();

	const data2 = await transport2.dump();

	expect(data2.id).toBe(transport2.id);
	expect(data2.direct).toBe(false);
	expect(data2.tuple).toEqual(transport2.tuple);
	expect(data2.rtcpTuple).toEqual(transport2.rtcpTuple);
	expect(data2.sctpState).toBeUndefined();
}, 2000);

test('router.createPlainTransport() with wrong arguments rejects with TypeError', async () =>
{
	await expect(router.createPlainTransport({}))
		.rejects
		.toThrow(TypeError);

	await expect(router.createPlainTransport({ listenIp: '123' }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createPlainTransport({ listenIp: [ '127.0.0.1' ] }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createPlainTransport(
		{
			listenIp : '127.0.0.1',
			appData  : 'NOT-AN-OBJECT'
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('router.createPlainTransport() with enableSrtp succeeds', async () =>
{
	const transport1 = await router.createPlainTransport(
		{
			listenIp   : '127.0.0.1',
			enableSrtp : true
		});

	expect(transport1.id).toBeType('string');
	expect(transport1.srtpParameters).toBeType('object');
	expect(transport1.srtpParameters.cryptoSuite).toBe('AES_CM_128_HMAC_SHA1_80');
	expect(transport1.srtpParameters.keyBase64.length).toBe(40);

	// Missing srtpParameters.
	await expect(transport1.connect(
		{
			ip   : '127.0.0.2',
			port : 9999
		}))
		.rejects
		.toThrow(TypeError);

	// Invalid srtpParameters.
	await expect(transport1.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters : 1
		}))
		.rejects
		.toThrow(TypeError);

	// Missing srtpParameters.cryptoSuite.
	await expect(transport1.connect(
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
	await expect(transport1.connect(
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
	await expect(transport1.connect(
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
	await expect(transport1.connect(
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
	await expect(transport1.connect(
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

	// Valid srtpParameters. And let's update the crypto suite.
	await expect(transport1.connect(
		{
			ip             : '127.0.0.2',
			port           : 9999,
			srtpParameters :
			{
				cryptoSuite : 'AES_CM_128_HMAC_SHA1_32',
				keyBase64   : 'ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv'
			}
		}))
		.resolves
		.toBeUndefined();

	expect(transport1.srtpParameters.cryptoSuite).toBe('AES_CM_128_HMAC_SHA1_32');

	transport1.close();
}, 2000);

test('router.createPlainTransport() with non bindable IP rejects with Error', async () =>
{
	await expect(router.createPlainTransport({ listenIp: '8.8.8.8' }))
		.rejects
		.toThrow(Error);
}, 2000);

test('plainTransport.getStats() succeeds', async () =>
{
	const data = await transport.getStats();

	expect(data).toBeType('array');
	expect(data.length).toBe(1);
	expect(data[0].type).toBe('plain-rtp-transport');
	expect(data[0].transportId).toBeType('string');
	expect(data[0].timestamp).toBeType('number');
	expect(data[0].bytesReceived).toBe(0);
	expect(data[0].recvBitrate).toBe(0);
	expect(data[0].bytesSent).toBe(0);
	expect(data[0].sendBitrate).toBe(0);
	expect(data[0].rtpBytesReceived).toBe(0);
	expect(data[0].rtpRecvBitrate).toBe(0);
	expect(data[0].rtpBytesSent).toBe(0);
	expect(data[0].rtpSendBitrate).toBe(0);
	expect(data[0].rtxBytesReceived).toBe(0);
	expect(data[0].rtxRecvBitrate).toBe(0);
	expect(data[0].rtxBytesSent).toBe(0);
	expect(data[0].rtxSendBitrate).toBe(0);
	expect(data[0].probationBytesSent).toBe(0);
	expect(data[0].probationSendBitrate).toBe(0);
	expect(data[0].tuple).toBeType('object');
	expect(data[0].tuple.localIp).toBe('4.4.4.4');
	expect(data[0].tuple.localPort).toBeType('number');
	expect(data[0].tuple.protocol).toBe('udp');
	expect(data[0].rtcpTuple).toBeUndefined();
	expect(data[0].recvBitrate).toBe(0);
	expect(data[0].sendBitrate).toBe(0);
}, 2000);

test('plainTransport.connect() succeeds', async () =>
{
	await expect(transport.connect({ ip: '1.2.3.4', port: 1234, rtcpPort: 1235 }))
		.resolves
		.toBeUndefined();

	// Must fail if connected.
	await expect(transport.connect({ ip: '1.2.3.4', port: 1234, rtcpPort: 1235 }))
		.rejects
		.toThrow(Error);

	expect(transport.tuple.remoteIp).toBe('1.2.3.4');
	expect(transport.tuple.remotePort).toBe(1234);
	expect(transport.tuple.protocol).toBe('udp');
	expect(transport.rtcpTuple.remoteIp).toBe('1.2.3.4');
	expect(transport.rtcpTuple.remotePort).toBe(1235);
	expect(transport.rtcpTuple.protocol).toBe('udp');
}, 2000);

test('plainTransport.connect() with wrong arguments rejects with TypeError', async () =>
{
	// No SRTP enabled so passing srtpParameters must fail.
	await expect(transport.connect(
		{
			ip             : '127.0.0.2',
			port           : 9998,
			rtcpPort       : 9999,
			srtpParameters :
			{
				cryptoSuite : 'AES_CM_128_HMAC_SHA1_80',
				keyBase64   : 'ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv'
			}
		}))
		.rejects
		.toThrow(TypeError);

	await expect(transport.connect({}))
		.rejects
		.toThrow(TypeError);

	await expect(transport.connect({ ip: '::::1234' }))
		.rejects
		.toThrow(TypeError);

	await expect(transport.connect({ ip: '127.0.0.1', port: 1234, __rtcpPort: 1235 }))
		.rejects
		.toThrow(TypeError);

	await expect(transport.connect({ ip: '127.0.0.1', __port: 'chicken', rtcpPort: 1235 }))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('PlainTransport methods reject if closed', async () =>
{
	const onObserverClose = jest.fn();

	transport.observer.once('close', onObserverClose);
	transport.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport.closed).toBe(true);

	await expect(transport.dump())
		.rejects
		.toThrow(Error);

	await expect(transport.getStats())
		.rejects
		.toThrow(Error);

	await expect(transport.connect({}))
		.rejects
		.toThrow(Error);
}, 2000);

test('PlainTransport emits "routerclose" if Router is closed', async () =>
{
	// We need different Router and PlainTransport instances here.
	const router2 = await worker.createRouter({ mediaCodecs });
	const transport2 =
		await router2.createPlainTransport({ listenIp: '127.0.0.1' });
	const onObserverClose = jest.fn();

	transport2.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		transport2.on('routerclose', resolve);
		router2.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport2.closed).toBe(true);
}, 2000);

test('PlainTransport emits "routerclose" if Worker is closed', async () =>
{
	const onObserverClose = jest.fn();

	transport.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		transport.on('routerclose', resolve);
		worker.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport.closed).toBe(true);
}, 2000);
