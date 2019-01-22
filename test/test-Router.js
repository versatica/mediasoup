const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;
const { InvalidStateError } = require('../lib/errors');

expect.extend({ toBeType });

let worker;

beforeEach(() => worker && !worker.closed && worker.close());
afterEach(() => worker && !worker.closed && worker.close());

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

test('worker.createRouter() succeeds', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });

	expect(router.id).toBeType('string');
	expect(router.closed).toBe(false);
	expect(router.rtpCapabilities).toBeType('object');

	worker.close();

	expect(router.closed).toBe(true);
}, 1000);

test('worker.createRouter() without mediaCodecs rejects with TypeError', async () =>
{
	worker = await createWorker();

	await expect(worker.createRouter())
		.rejects
		.toThrow(TypeError);

	worker.close();
}, 1000);

test('worker.createRouter() rejects with InvalidStateError if worker is closed', async () =>
{
	worker = await createWorker();

	worker.close();

	await expect(worker.createRouter({ mediaCodecs }))
		.rejects
		.toThrow(InvalidStateError);
}, 1000);

test('Router emits "workerclose" if Worker is closed', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });

	await new Promise((resolve) =>
	{
		router.on('workerclose', resolve);

		worker.close();
	});

	expect(router.closed).toBe(true);
}, 1000);

test('router.createWebRtcTransport() succeeds', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });
	const transport = await router.createWebRtcTransport(
		{
			listenIps :
			[
				{ ip: '127.0.0.1', announcedIp: '9.9.9.1' },
				{ ip: '0.0.0.0', announcedIp: '9.9.9.2' },
				{ ip: '127.0.0.1' }
			],
			enableTcp : true,
			preferUdp : true,
			appData   : { foo: 'bar' }
		});

	expect(transport.id).toBeType('string');
	expect(transport.closed).toBe(false);
	expect(transport.appData).toEqual({ foo: 'bar' });
	expect(transport.iceRole).toBe('controlled');
	expect(transport.iceLocalParameters).toBeType('object');
	expect(transport.iceLocalParameters.iceLite).toBe(true);
	expect(transport.iceLocalParameters.usernameFragment).toBeType('string');
	expect(transport.iceLocalParameters.password).toBeType('string');
	expect(transport.iceLocalCandidates).toBeType('array');
	expect(transport.iceLocalCandidates.length).toBe(6);

	const iceCandidates = transport.iceLocalCandidates;

	expect(iceCandidates[0].ip).toBe('9.9.9.1');
	expect(iceCandidates[0].protocol).toBe('udp');
	expect(iceCandidates[0].type).toBe('host');
	expect(iceCandidates[0].tcpType).toBe(undefined);
	expect(iceCandidates[1].ip).toBe('9.9.9.1');
	expect(iceCandidates[1].protocol).toBe('tcp');
	expect(iceCandidates[1].type).toBe('host');
	expect(iceCandidates[1].tcpType).toBe('passive');
	expect(iceCandidates[2].ip).toBe('9.9.9.2');
	expect(iceCandidates[2].protocol).toBe('udp');
	expect(iceCandidates[2].type).toBe('host');
	expect(iceCandidates[2].tcpType).toBe(undefined);
	expect(iceCandidates[3].ip).toBe('9.9.9.2');
	expect(iceCandidates[3].protocol).toBe('tcp');
	expect(iceCandidates[3].type).toBe('host');
	expect(iceCandidates[3].tcpType).toBe('passive');
	expect(iceCandidates[4].ip).toBe('127.0.0.1');
	expect(iceCandidates[4].protocol).toBe('udp');
	expect(iceCandidates[4].type).toBe('host');
	expect(iceCandidates[4].tcpType).toBe(undefined);
	expect(iceCandidates[5].ip).toBe('127.0.0.1');
	expect(iceCandidates[5].protocol).toBe('tcp');
	expect(iceCandidates[5].type).toBe('host');
	expect(iceCandidates[5].tcpType).toBe('passive');
	expect(iceCandidates[0].priority).toBeGreaterThan(iceCandidates[1].priority);
	expect(iceCandidates[2].priority).toBeGreaterThan(iceCandidates[1].priority);
	expect(iceCandidates[2].priority).toBeGreaterThan(iceCandidates[3].priority);
	expect(iceCandidates[4].priority).toBeGreaterThan(iceCandidates[3].priority);
	expect(iceCandidates[4].priority).toBeGreaterThan(iceCandidates[5].priority);
	expect(transport.iceState).toBe('new');
	expect(transport.iceSelectedTuple).toBe(undefined);
	expect(transport.dtlsLocalParameters).toBeType('object');
	expect(transport.dtlsLocalParameters.fingerprints).toBeType('array');
	expect(transport.dtlsLocalParameters.role).toBe('auto');
	expect(transport.dtlsState).toBe('new');

	const data = await transport.dump();

	expect(data.id).toBe(transport.id);
	expect(data.iceRole).toBe(transport.iceRole);
	expect(data.iceLocalParameters).toEqual(transport.iceLocalParameters);
	expect(data.iceLocalCandidates).toEqual(transport.iceLocalCandidates);
	expect(data.iceState).toBe(transport.iceState);
	expect(data.iceSelectedTuple).toEqual(transport.iceSelectedTuple);
	expect(data.dtlsLocalParameters).toEqual(transport.dtlsLocalParameters);
	expect(data.dtlsState).toBe(transport.dtlsState);
	expect(data.rtpHeaderExtensions).toBeType('object');
	expect(data.rtpListener).toBeType('object');

	expect(router.getTransportById(transport.id)).toBe(transport);
	transport.close();
	expect(transport.closed).toBe(true);
	expect(router.getTransportById(transport.id)).toBe(undefined);

	await expect(router.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] }))
		.resolves
		.toBeType('object');

	worker.close();
}, 1000);

test('router.createWebRtcTransport() with wrong options rejects with TypeError', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });

	await expect(router.createWebRtcTransport())
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport({ listenIps: [] }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport({ listenIps: [ 123 ] }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport({ listenIps: '127.0.0.1' }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ],
			appData   : 'NOT-AN-OBJECT'
		}))
		.rejects
		.toThrow(TypeError);

	worker.close();
}, 1000);

test('router.createWebRtcTransport() with non bindable IP rejects with Error', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });

	await expect(router.createWebRtcTransport({ listenIps: [ '8.8.8.8' ] }))
		.rejects
		.toThrow(Error);

	worker.close();
}, 1000);

test('router.createPlainRtpTransport() succeeds', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });
	const transport1 = await router.createPlainRtpTransport(
		{
			listenIp : { ip: '127.0.0.1', announcedIp: '9.9.9.1' },
			rtcpMux  : true,
			appData  : { foo: 'bar' }
		});

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
	expect(data1.tuple).toEqual(transport1.tuple);
	expect(data1.rtcpTuple).toEqual(transport1.rtcpTuple);
	expect(data1.rtpHeaderExtensions).toBeType('object');
	expect(data1.rtpListener).toBeType('object');

	expect(router.getTransportById(transport1.id)).toBe(transport1);
	transport1.close();
	expect(transport1.closed).toBe(true);
	expect(router.getTransportById(transport1.id)).toBe(undefined);

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

	expect(router.getTransportById(transport2.id)).toBe(transport2);

	worker.close();

	expect(transport2.closed).toBe(true);
	expect(router.getTransportById(transport2.id)).toBe(undefined);
}, 1000);

test('router.createPlainRtpTransport() with wrong options rejects with TypeError', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });

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

	worker.close();
}, 1000);

test('router.createPlainRtpTransport() with non bindable IP rejects with Error', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });

	await expect(router.createPlainRtpTransport({ listenIp: '8.8.8.8' }))
		.rejects
		.toThrow(Error);

	worker.close();
}, 1000);
