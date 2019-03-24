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
		kind         : 'video',
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
});

afterAll(() => worker.close());

beforeEach(async () =>
{
	transport = await router.createPlainRtpTransport(
		{
			listenIp : { ip: '127.0.0.1', announcedIp: '4.4.4.4' },
			rtcpMux  : false
		});
});

afterEach(() => transport.close());

test('router.createPlainRtpTransport() succeeds', async () =>
{
	await expect(router.dump())
		.resolves
		.toMatchObject({ transportIds: [ transport.id ] });

	const onObserverNewTransport = jest.fn();

	router.observer.once('newtransport', onObserverNewTransport);

	// Create a separate transport here.
	const transport1 = await router.createPlainRtpTransport(
		{
			listenIp : { ip: '127.0.0.1', announcedIp: '9.9.9.1' },
			rtcpMux  : true,
			appData  : { foo: 'bar' }
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
	expect(transport1.rtcpTuple).toBe(undefined);

	const data1 = await transport1.dump();

	expect(data1.id).toBe(transport1.id);
	expect(data1.producerIds).toEqual([]);
	expect(data1.consumerIds).toEqual([]);
	expect(data1.tuple).toEqual(transport1.tuple);
	expect(data1.rtcpTuple).toEqual(transport1.rtcpTuple);
	expect(data1.rtpHeaderExtensions).toBeType('object');
	expect(data1.rtpListener).toBeType('object');

	transport1.close();
	expect(transport1.closed).toBe(true);

	await expect(router.createPlainRtpTransport({ listenIp: '127.0.0.1' }))
		.resolves
		.toBeType('object');

	const transport2 = await router.createPlainRtpTransport(
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

	const data2 = await transport2.dump();

	expect(data2.id).toBe(transport2.id);
	expect(data2.tuple).toEqual(transport2.tuple);
	expect(data2.rtcpTuple).toEqual(transport2.rtcpTuple);
}, 2000);

test('router.createPlainRtpTransport() with wrong arguments rejects with TypeError', async () =>
{
	await expect(router.createPlainRtpTransport())
		.rejects
		.toThrow(TypeError);

	await expect(router.createPlainRtpTransport({ listenIp: '123' }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createPlainRtpTransport({ listenIp: [ '127.0.0.1' ] }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createPlainRtpTransport(
		{
			listenIp : '127.0.0.1',
			appData  : 'NOT-AN-OBJECT'
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('router.createPlainRtpTransport() with non bindable IP rejects with Error', async () =>
{
	await expect(router.createPlainRtpTransport({ listenIp: '8.8.8.8' }))
		.rejects
		.toThrow(Error);
}, 2000);

test('plaintRtpTransport.getStats() succeeds', async () =>
{
	const data = await transport.getStats();

	expect(data).toBeType('array');
	expect(data.length).toBe(1);
	expect(data[0].type).toBe('transport');
	expect(data[0].transportId).toBeType('string');
	expect(data[0].timestamp).toBeType('number');
	expect(data[0].bytesReceived).toBe(0);
	expect(data[0].bytesSent).toBe(0);
	expect(data[0].tuple).toBe(undefined);
	expect(data[0].rtcpTuple).toBe(undefined);
}, 2000);

test('plaintRtpTransport.connect() succeeds', async () =>
{
	await expect(transport.connect({ ip: '1.2.3.4', port: 1234, rtcpPort: 1235 }))
		.resolves
		.toBe(undefined);

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

test('plaintRtpTransport.connect() with wrong arguments rejects with TypeError', async () =>
{
	await expect(transport.connect())
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

test('PlaintRtpTransport methods reject if closed', async () =>
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

	await expect(transport.connect())
		.rejects
		.toThrow(Error);
}, 2000);

test('PlaintRtpTransport emits "routerclose" if Router is closed', async () =>
{
	// We need different Router and PlaintRtpTransport instances here.
	const router2 = await worker.createRouter({ mediaCodecs });
	const transport2 =
		await router2.createPlainRtpTransport({ listenIp: '127.0.0.1' });
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

test('PlaintRtpTransport emits "routerclose" if Worker is closed', async () =>
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
