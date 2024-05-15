import * as process from 'node:process';
import * as path from 'node:path';
import * as mediasoup from '../';
import { InvalidStateError } from '../errors';

test('Worker.workerBin matches mediasoup-worker absolute path', () => {
	const workerBin = process.env.MEDIASOUP_WORKER_BIN
		? process.env.MEDIASOUP_WORKER_BIN
		: process.env.MEDIASOUP_BUILDTYPE === 'Debug'
			? path.join(
					__dirname,
					'..',
					'..',
					'..',
					'worker',
					'out',
					'Debug',
					'mediasoup-worker'
				)
			: path.join(
					__dirname,
					'..',
					'..',
					'..',
					'worker',
					'out',
					'Release',
					'mediasoup-worker'
				);

	expect(mediasoup.workerBin).toBe(workerBin);
});

test('createWorker() succeeds', async () => {
	const onObserverNewWorker = jest.fn();

	mediasoup.observer.once('newworker', onObserverNewWorker);

	const worker1 = await mediasoup.createWorker();

	expect(onObserverNewWorker).toHaveBeenCalledTimes(1);
	expect(onObserverNewWorker).toHaveBeenCalledWith(worker1);
	expect(worker1.constructor.name).toBe('Worker');
	expect(typeof worker1.pid).toBe('number');
	expect(worker1.closed).toBe(false);

	worker1.close();

	expect(worker1.closed).toBe(true);

	const worker2 = await mediasoup.createWorker<{ foo: number; bar?: string }>({
		logLevel: 'debug',
		logTags: ['info'],
		rtcMinPort: 0,
		rtcMaxPort: 9999,
		dtlsCertificateFile: path.join(__dirname, 'data', 'dtls-cert.pem'),
		dtlsPrivateKeyFile: path.join(__dirname, 'data', 'dtls-key.pem'),
		libwebrtcFieldTrials: 'WebRTC-Bwe-AlrLimitedBackoff/Disabled/',
		appData: { foo: 456 },
	});

	expect(worker2.constructor.name).toBe('Worker');
	expect(typeof worker2.pid).toBe('number');
	expect(worker2.closed).toBe(false);
	expect(worker2.appData).toEqual({ foo: 456 });

	worker2.close();

	expect(worker2.closed).toBe(true);
}, 2000);

test('createWorker() with wrong settings rejects with TypeError', async () => {
	// @ts-ignore
	await expect(mediasoup.createWorker({ logLevel: 'chicken' })).rejects.toThrow(
		TypeError
	);

	await expect(
		mediasoup.createWorker({ rtcMinPort: 1000, rtcMaxPort: 999 })
	).rejects.toThrow(TypeError);

	// Port is from 0 to 65535.
	await expect(
		mediasoup.createWorker({ rtcMinPort: 1000, rtcMaxPort: 65536 })
	).rejects.toThrow(TypeError);

	await expect(
		mediasoup.createWorker({ dtlsCertificateFile: '/notfound/cert.pem' })
	).rejects.toThrow(TypeError);

	await expect(
		mediasoup.createWorker({ dtlsPrivateKeyFile: '/notfound/priv.pem' })
	).rejects.toThrow(TypeError);

	await expect(
		// @ts-ignore
		mediasoup.createWorker({ appData: 'NOT-AN-OBJECT' })
	).rejects.toThrow(TypeError);
}, 2000);

test('worker.updateSettings() succeeds', async () => {
	const worker = await mediasoup.createWorker();

	await expect(
		worker.updateSettings({ logLevel: 'debug', logTags: ['ice'] })
	).resolves.toBeUndefined();

	worker.close();
}, 2000);

test('worker.updateSettings() with wrong settings rejects with TypeError', async () => {
	const worker = await mediasoup.createWorker();

	// @ts-ignore
	await expect(worker.updateSettings({ logLevel: 'chicken' })).rejects.toThrow(
		TypeError
	);

	worker.close();
}, 2000);

test('worker.updateSettings() rejects with InvalidStateError if closed', async () => {
	const worker = await mediasoup.createWorker();

	worker.close();

	await expect(worker.updateSettings({ logLevel: 'error' })).rejects.toThrow(
		InvalidStateError
	);
}, 2000);

test('worker.dump() succeeds', async () => {
	const worker = await mediasoup.createWorker();

	await expect(worker.dump()).resolves.toMatchObject({
		pid: worker.pid,
		webRtcServerIds: [],
		routerIds: [],
		channelMessageHandlers: {
			channelRequestHandlers: [],
			channelNotificationHandlers: [],
		},
	});

	worker.close();
}, 2000);

test('worker.dump() rejects with InvalidStateError if closed', async () => {
	const worker = await mediasoup.createWorker();

	worker.close();

	await expect(worker.dump()).rejects.toThrow(InvalidStateError);
}, 2000);

test('worker.getResourceUsage() succeeds', async () => {
	const worker = await mediasoup.createWorker();

	await expect(worker.getResourceUsage()).resolves.toMatchObject({});

	worker.close();
}, 2000);

test('worker.close() succeeds', async () => {
	const worker = await mediasoup.createWorker({ logLevel: 'warn' });
	const onObserverClose = jest.fn();

	worker.observer.once('close', onObserverClose);
	worker.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(worker.closed).toBe(true);
}, 2000);
