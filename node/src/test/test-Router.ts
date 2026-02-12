import * as mediasoup from '../';
import { enhancedOnce } from '../enhancedEvents';
import type { WorkerImpl } from '../Worker';
import type { WorkerEvents, RouterEvents } from '../types';
import { InvalidStateError, UnsupportedError } from '../errors';
import * as utils from '../utils';

type TestContext = {
	mediaCodecs: mediasoup.types.RouterRtpCodecCapability[];
	unsupportedMediaCodecs: mediasoup.types.RouterRtpCodecCapability[];
	worker?: mediasoup.types.Worker;
};

const ctx: TestContext = {
	mediaCodecs: utils.deepFreeze<mediasoup.types.RouterRtpCodecCapability[]>([
		{
			kind: 'audio',
			mimeType: 'audio/opus',
			clockRate: 48000,
			channels: 2,
			parameters: {
				useinbandfec: 1,
				foo: 'bar',
			},
		},
		{
			kind: 'video',
			mimeType: 'video/VP8',
			clockRate: 90000,
		},
		{
			kind: 'video',
			mimeType: 'video/H264',
			clockRate: 90000,
			parameters: {
				'level-asymmetry-allowed': 1,
				'packetization-mode': 1,
				'profile-level-id': '4d0032',
			},
			rtcpFeedback: [], // Will be ignored.
		},
	]),
	unsupportedMediaCodecs: utils.deepFreeze<
		mediasoup.types.RouterRtpCodecCapability[]
	>([
		{
			kind: 'audio',
			mimeType: 'audio/x-aiff',
			clockRate: 8000,
			channels: 1,
		},
		{
			kind: 'video',
			mimeType: 'video/3gpp',
			clockRate: 90000,
		},
	]),
};

beforeEach(async () => {
	ctx.worker = await mediasoup.createWorker();
});

afterEach(async () => {
	ctx.worker?.close();

	if (ctx.worker?.subprocessClosed === false) {
		await enhancedOnce<WorkerEvents>(ctx.worker, 'subprocessclose');
	}
});

test('worker.createRouter() succeeds', async () => {
	const onObserverNewRouter = jest.fn();

	ctx.worker!.observer.once('newrouter', onObserverNewRouter);

	const router = await ctx.worker!.createRouter<{ foo: number; bar?: string }>({
		mediaCodecs: ctx.mediaCodecs,
		appData: { foo: 123 },
	});

	expect(onObserverNewRouter).toHaveBeenCalledTimes(1);
	expect(onObserverNewRouter).toHaveBeenCalledWith(router);
	expect(typeof router.id).toBe('string');
	expect(router.closed).toBe(false);
	expect(typeof router.rtpCapabilities).toBe('object');
	expect(Array.isArray(router.rtpCapabilities.codecs)).toBe(true);
	// 3 codecs + 2 RTX codecs.
	expect(router.rtpCapabilities.codecs?.length).toBe(5);
	expect(Array.isArray(router.rtpCapabilities.headerExtensions)).toBe(true);
	expect(router.appData).toEqual({ foo: 123 });

	expect(() => (router.appData = { foo: 222, bar: 'BBB' })).not.toThrow();

	await expect(ctx.worker!.dump()).resolves.toMatchObject({
		pid: ctx.worker!.pid,
		webRtcServerIds: [],
		routerIds: [router.id],
		channelMessageHandlers: {
			channelRequestHandlers: [router.id],
			channelNotificationHandlers: [],
		},
	});

	await expect(router.dump()).resolves.toMatchObject({
		id: router.id,
		transportIds: [],
		rtpObserverIds: [],
		mapProducerIdConsumerIds: {},
		mapConsumerIdProducerId: {},
		mapProducerIdObserverIds: {},
		mapDataProducerIdDataConsumerIds: {},
		mapDataConsumerIdDataProducerId: {},
	});

	// API not exposed in the interface.
	expect((ctx.worker! as WorkerImpl).routersForTesting.size).toBe(1);

	ctx.worker!.close();

	expect(router.closed).toBe(true);

	// API not exposed in the interface.
	expect((ctx.worker! as WorkerImpl).routersForTesting.size).toBe(0);
}, 2000);

test('worker.createRouter() with invalid codecs rejects with UnsupportedError', async () => {
	await expect(
		ctx.worker!.createRouter({ mediaCodecs: ctx.unsupportedMediaCodecs })
	).rejects.toThrow(UnsupportedError);
}, 2000);

test('worker.createRouter() with wrong arguments rejects with TypeError', async () => {
	// @ts-expect-error --- Testing purposes.
	await expect(ctx.worker!.createRouter({ mediaCodecs: {} })).rejects.toThrow(
		TypeError
	);

	await expect(
		// @ts-expect-error --- Testing purposes.
		ctx.worker!.createRouter({ appData: 'NOT-AN-OBJECT' })
	).rejects.toThrow(TypeError);
}, 2000);

test('worker.createRouter() rejects with InvalidStateError if Worker is closed', async () => {
	ctx.worker!.close();

	await expect(
		ctx.worker!.createRouter({ mediaCodecs: ctx.mediaCodecs })
	).rejects.toThrow(InvalidStateError);
}, 2000);

test('router.updateMediaCodecs() succeeds', async () => {
	const router = await ctx.worker!.createRouter({
		mediaCodecs: ctx.mediaCodecs,
	});

	expect(typeof router.rtpCapabilities).toBe('object');
	expect(Array.isArray(router.rtpCapabilities.codecs)).toBe(true);
	// 3 codecs + 2 RTX codecs.
	expect(router.rtpCapabilities.codecs?.length).toBe(5);
	expect(Array.isArray(router.rtpCapabilities.headerExtensions)).toBe(true);

	router.updateMediaCodecs([]);

	expect(typeof router.rtpCapabilities).toBe('object');
	expect(Array.isArray(router.rtpCapabilities.codecs)).toBe(true);
	expect(router.rtpCapabilities.codecs?.length).toBe(0);
	expect(Array.isArray(router.rtpCapabilities.headerExtensions)).toBe(true);
}, 2000);

test('router.close() succeeds', async () => {
	const router = await ctx.worker!.createRouter({
		mediaCodecs: ctx.mediaCodecs,
	});

	const onObserverClose = jest.fn();

	router.observer.once('close', onObserverClose);
	router.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(router.closed).toBe(true);
}, 2000);

test('Router emits "workerclose" if Worker is closed', async () => {
	const router = await ctx.worker!.createRouter({
		mediaCodecs: ctx.mediaCodecs,
	});

	const onObserverClose = jest.fn();

	router.observer.once('close', onObserverClose);

	const promise = enhancedOnce<RouterEvents>(router, 'workerclose');

	ctx.worker!.close();
	await promise;

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(router.closed).toBe(true);
}, 2000);
