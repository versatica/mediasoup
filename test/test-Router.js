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
				{ ip: '0.0.0.0', announcedIp: '9.9.9.2' }
			],
			enableTcp : true,
			preferdp  : true,
			appData   : { foo: 'bar' }
		});

	expect(transport.appData).toEqual({ foo: 'bar' });

	const data = await transport.dump();

	expect(data.id).toBeType('string');
	expect(data.iceRole).toBe('controlled');
	expect(data.iceLocalParameters).toBeType('object');
	expect(data.iceLocalParameters.iceLite).toBe(true);
	expect(data.iceLocalParameters.usernameFragment).toBeType('string');
	expect(data.iceLocalParameters.password).toBeType('string');
	expect(data.iceLocalCandidates).toBeType('array');
	expect(data.iceLocalCandidates.length).toBe(4);
	expect(data.iceState).toBe('new');
	expect(data.iceSelectedTuple).toBe(undefined);
	expect(data.dtlsLocalParameters).toBeType('object');
	expect(data.dtlsLocalParameters.fingerprints).toBeType('array');
	expect(data.dtlsLocalParameters.role).toBe('auto');
	expect(data.dtlsState).toBe('new');
	expect(data.rtpHeaderExtensions).toBeType('object');
	expect(data.rtpListener).toBeType('object');

	worker.close();
}, 1000);

test('router.createPlainRtpTransport() succeeds', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter({ mediaCodecs });
	const transport = await router.createPlainRtpTransport(
		{
			listenIp : { ip: '127.0.0.1', announcedIp: '9.9.9.1' },
			rtcpMux  : true,
			appData  : { foo: 'bar' }
		});

	expect(transport.appData).toEqual({ foo: 'bar' });

	const data = await transport.dump();

	expect(data.id).toBeType('string');
	expect(data.tuple).toBeType('object');
	expect(data.tuple.localIp).toBeType('string');
	expect(data.tuple.localPort).toBeType('number');
	expect(data.tuple.transport).toBe('udp');
	expect(data.rtcpTuple).toBe(undefined);
	expect(data.rtpHeaderExtensions).toBeType('object');
	expect(data.rtpListener).toBeType('object');

	worker.close();
}, 1000);
