import * as mediasoup from '../';
import * as utils from '../utils';

type TestContext = {
	mediaCodecs: mediasoup.types.RtpCodecCapability[];
	worker?: mediasoup.types.Worker;
	router?: mediasoup.types.Router;
};

const ctx: TestContext = {
	mediaCodecs: utils.deepFreeze([
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
	]),
};

beforeEach(async () => {
	ctx.worker = await mediasoup.createWorker();
	ctx.router = await ctx.worker.createRouter({ mediaCodecs: ctx.mediaCodecs });
});

afterEach(async () => {
	ctx.worker?.close();

	if (ctx.worker?.subprocessClosed === false) {
		await new Promise<void>(resolve =>
			ctx.worker?.on('subprocessclose', resolve)
		);
	}
});

test('router.createActiveSpeakerObserver() succeeds', async () => {
	const onObserverNewRtpObserver = jest.fn();

	ctx.router!.observer.once('newrtpobserver', onObserverNewRtpObserver);

	const activeSpeakerObserver = await ctx.router!.createActiveSpeakerObserver();

	expect(onObserverNewRtpObserver).toHaveBeenCalledTimes(1);
	expect(onObserverNewRtpObserver).toHaveBeenCalledWith(activeSpeakerObserver);
	expect(typeof activeSpeakerObserver.id).toBe('string');
	expect(activeSpeakerObserver.closed).toBe(false);
	expect(activeSpeakerObserver.paused).toBe(false);
	expect(activeSpeakerObserver.appData).toEqual({});

	await expect(ctx.router!.dump()).resolves.toMatchObject({
		rtpObserverIds: [activeSpeakerObserver.id],
	});
}, 2000);

test('router.createActiveSpeakerObserver() with wrong arguments rejects with TypeError', async () => {
	await expect(
		ctx.router!.createActiveSpeakerObserver(
			// @ts-ignore
			{ interval: false }
		)
	).rejects.toThrow(TypeError);

	await expect(
		ctx.router!.createActiveSpeakerObserver(
			// @ts-ignore
			{ appData: 'NOT-AN-OBJECT' }
		)
	).rejects.toThrow(TypeError);
}, 2000);

test('activeSpeakerObserver.pause() and resume() succeed', async () => {
	const activeSpeakerObserver = await ctx.router!.createActiveSpeakerObserver();

	await activeSpeakerObserver.pause();

	expect(activeSpeakerObserver.paused).toBe(true);

	await activeSpeakerObserver.resume();

	expect(activeSpeakerObserver.paused).toBe(false);
}, 2000);

test('activeSpeakerObserver.close() succeeds', async () => {
	const activeSpeakerObserver = await ctx.router!.createAudioLevelObserver({
		maxEntries: 8,
	});

	let dump = await ctx.router!.dump();

	expect(dump.rtpObserverIds.length).toBe(1);

	activeSpeakerObserver.close();

	expect(activeSpeakerObserver.closed).toBe(true);

	dump = await ctx.router!.dump();

	expect(dump.rtpObserverIds.length).toBe(0);
}, 2000);

test('ActiveSpeakerObserver emits "routerclose" if Router is closed', async () => {
	const activeSpeakerObserver = await ctx.router!.createAudioLevelObserver();

	await new Promise<void>(resolve => {
		activeSpeakerObserver.on('routerclose', resolve);
		ctx.router!.close();
	});

	expect(activeSpeakerObserver.closed).toBe(true);
}, 2000);

test('ActiveSpeakerObserver emits "routerclose" if Worker is closed', async () => {
	const activeSpeakerObserver = await ctx.router!.createAudioLevelObserver();

	await new Promise<void>(resolve => {
		activeSpeakerObserver.on('routerclose', resolve);
		ctx.worker!.close();
	});

	expect(activeSpeakerObserver.closed).toBe(true);
}, 2000);
