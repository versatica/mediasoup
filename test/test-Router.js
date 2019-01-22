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
