const process = require('process');
const path = require('path');
const { toBeType } = require('jest-tobetype');
const pkg = require('../package.json');
const mediasoup = require('../');
const { version, createWorker } = mediasoup;
const { InvalidStateError } = require('../lib/errors');

expect.extend({ toBeType });

let worker;

beforeEach(() => worker && !worker.closed && worker.close());
afterEach(() => worker && !worker.closed && worker.close());

test('mediasoup exposes a version property', () =>
{
	expect(version).toBeType('string');
	expect(version).toBe(pkg.version);
}, 500);

test('createWorker() succeeds', async () =>
{
	worker = await createWorker();
	expect(worker).toBeType('object');
	expect(worker.pid).toBeType('number');
	expect(worker.closed).toBe(false);

	worker.close();
	expect(worker.closed).toBe(true);

	worker = await createWorker(
		{
			logLevel            : 'debug',
			logTags             : [ 'info' ],
			rtcMinPort          : 0,
			rtcMaxPort          : 9999,
			dtlsCertificateFile : path.join(__dirname, 'data', 'dtls-cert.pem'),
			dtlsPrivateKeyFile  : path.join(__dirname, 'data', 'dtls-key.pem')
		});
	expect(worker).toBeType('object');
	expect(worker.pid).toBeType('number');
	expect(worker.closed).toBe(false);

	worker.close();
	expect(worker.closed).toBe(true);
}, 500);

test('createWorker() with wrong settings rejects with TypeError', async () =>
{
	await expect(createWorker({ logLevel: 'chicken' }))
		.rejects
		.toThrow(TypeError);

	await expect(createWorker({ rtcMinPort: 1000, rtcMaxPort: 1000 }))
		.rejects
		.toThrow(TypeError);

	// Port is from 0 to 65535.
	await expect(createWorker({ rtcMinPort: 1000, rtcMaxPort: 65536 }))
		.rejects
		.toThrow(TypeError);

	await expect(createWorker({ dtlsCertificateFile: '/notfound/cert.pem' }))
		.rejects
		.toThrow(TypeError);

	await expect(createWorker({ dtlsPrivateKeyFile: '/notfound/priv.pem' }))
		.rejects
		.toThrow(TypeError);
}, 500);

test('worker.updateSettings() succeeds', async () =>
{
	worker = await createWorker();

	await expect(worker.updateSettings({ logLevel: 'debug', logTags: [ 'ice' ] }))
		.resolves
		.toBe(undefined);

	worker.close();
}, 500);

test('worker.updateSettings() with wrong settings rejects', async () =>
{
	worker = await createWorker();

	await expect(worker.updateSettings({ logLevel: 'chicken' }))
		.rejects
		.toThrow(Error);

	worker.close();
}, 500);

test('worker.updateSettings() rejects with InvalidStateError if closed', async () =>
{
	worker = await createWorker();
	worker.close();

	await expect(worker.updateSettings({ logLevel: 'error' }))
		.rejects
		.toThrow(InvalidStateError);

	worker.close();
}, 500);

test('worker.dump() succeeds', async () =>
{
	worker = await createWorker();

	await expect(worker.dump())
		.resolves
		.toEqual({ pid: worker.pid, routerIds: [] });

	worker.close();
}, 500);

test('worker.dump() rejects with InvalidStateError if closed', async () =>
{
	worker = await createWorker();
	worker.close();

	await expect(worker.dump({ logLevel: 'error' }))
		.rejects
		.toThrow(InvalidStateError);

	worker.close();
}, 500);

test('Worker emits "died" if worker subprocess died unexpectedly', async () =>
{
	worker = await createWorker({ logLevel: 'warn' });

	await new Promise((resolve) =>
	{
		worker.on('died', resolve);

		process.kill(worker.pid, 'SIGINT');
	});

	worker = await createWorker({ logLevel: 'warn' });

	await new Promise((resolve) =>
	{
		worker.on('died', resolve);

		process.kill(worker.pid, 'SIGTERM');
	});

	worker = await createWorker({ logLevel: 'warn' });

	await new Promise((resolve) =>
	{
		worker.on('died', resolve);

		process.kill(worker.pid, 'SIGKILL');
	});
}, 2000);

test('worker process ignores PIPE, HUP, ALRM, USR1 and USR2 signals', async () =>
{
	worker = await createWorker({ logLevel: 'warn' });

	await new Promise((resolve, reject) =>
	{
		worker.on('died', reject);

		process.kill(worker.pid, 'SIGPIPE');
		process.kill(worker.pid, 'SIGHUP');
		process.kill(worker.pid, 'SIGALRM');
		process.kill(worker.pid, 'SIGUSR1');
		process.kill(worker.pid, 'SIGUSR2');

		setTimeout(() =>
		{
			expect(worker.closed).toBe(false);

			worker.close();
			resolve();
		}, 250);
	});
}, 2000);
