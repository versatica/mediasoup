import * as mediasoup from '../';
import { InvalidStateError } from '../errors';

let worker: mediasoup.types.Worker;

beforeEach(() => worker && !worker.closed && worker.close());
afterEach(() => worker && !worker.closed && worker.close());

const mediaCodecs: mediasoup.types.RtpCodecCapability[] =
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
			'profile-level-id'        : '4d0032'
		},
		rtcpFeedback : [] // Will be ignored.
	}
];

test('worker.createRouter() succeeds', async () =>
{
	worker = await mediasoup.createWorker();

	const onObserverNewRouter = jest.fn();

	worker.observer.once('newrouter', onObserverNewRouter);

	const router = await worker.createRouter<{ foo: number; bar?: string }>(
		{
			mediaCodecs,
			appData : { foo: 123 }
		});

	expect(onObserverNewRouter).toHaveBeenCalledTimes(1);
	expect(onObserverNewRouter).toHaveBeenCalledWith(router);
	expect(typeof router.id).toBe('string');
	expect(router.closed).toBe(false);
	expect(typeof router.rtpCapabilities).toBe('object');
	expect(Array.isArray(router.rtpCapabilities.codecs)).toBe(true);
	expect(Array.isArray(router.rtpCapabilities.headerExtensions)).toBe(true);
	expect(router.appData).toEqual({ foo: 123 });

	expect(() => (router.appData = { foo: 222, bar: 'BBB' })).not.toThrow();

	await expect(worker.dump())
		.resolves
		.toMatchObject(
			{
				pid                    : worker.pid,
				webRtcServerIds        : [],
				routerIds              : [ router.id ],
				channelMessageHandlers :
				{
					channelRequestHandlers      : [ router.id ],
					channelNotificationHandlers : []
				}
			});

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				id                               : router.id,
				transportIds                     : [],
				rtpObserverIds                   : [],
				mapProducerIdConsumerIds         : {},
				mapConsumerIdProducerId          : {},
				mapProducerIdObserverIds         : {},
				mapDataProducerIdDataConsumerIds : {},
				mapDataConsumerIdDataProducerId  : {}
			});

	// Private API.
	expect(worker.routersForTesting.size).toBe(1);

	worker.close();

	expect(router.closed).toBe(true);

	// Private API.
	expect(worker.routersForTesting.size).toBe(0);
}, 2000);

test('worker.createRouter() with wrong arguments rejects with TypeError', async () =>
{
	worker = await mediasoup.createWorker();

	// @ts-ignore
	await expect(worker.createRouter({ mediaCodecs: {} }))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(worker.createRouter({ appData: 'NOT-AN-OBJECT' }))
		.rejects
		.toThrow(TypeError);

	worker.close();
}, 2000);

test('worker.createRouter() rejects with InvalidStateError if Worker is closed', async () =>
{
	worker = await mediasoup.createWorker();

	worker.close();

	await expect(worker.createRouter({ mediaCodecs }))
		.rejects
		.toThrow(InvalidStateError);
}, 2000);

test('router.close() succeeds', async () =>
{
	worker = await mediasoup.createWorker();

	const router = await worker.createRouter({ mediaCodecs });
	const onObserverClose = jest.fn();

	router.observer.once('close', onObserverClose);
	router.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(router.closed).toBe(true);
}, 2000);

test('Router emits "workerclose" if Worker is closed', async () =>
{
	worker = await mediasoup.createWorker();

	const router = await worker.createRouter({ mediaCodecs });
	const onObserverClose = jest.fn();

	router.observer.once('close', onObserverClose);

	await new Promise<void>((resolve) =>
	{
		router.on('workerclose', resolve);
		worker.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(router.closed).toBe(true);
}, 2000);
