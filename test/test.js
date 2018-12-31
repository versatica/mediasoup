const path = require('path');
const { toBeType } = require('jest-tobetype');
const pkg = require('../package.json');
const mediasoup = require('../');
const { version, createWorker } = mediasoup;

expect.extend({ toBeType });

test('mediasoup exposes a version property', () =>
{
	expect(version).toBeType('string');
	expect(version).toBe(pkg.version);
}, 500);

test('createWorker() succeeds', async () =>
{
	let worker;

	worker = await createWorker();
	expect(worker).toBeType('object');
	expect(worker.id).toBeType('string');
	expect(worker.closed).toBe(false);

	worker.close();
	expect(worker.closed).toBe(true);

	worker = await createWorker(
		{
			logger              : 'debug',
			logTags             : [ 'info' ],
			rtcMinPort          : 2000,
			rtcMaxPort          : 3000,
			dtlsCertificateFile : path.join(__dirname, 'data', 'dtls-cert.pem'),
			dtlsPrivateKeyFile  : path.join(__dirname, 'data', 'dtls-key.pem')
		});
	expect(worker).toBeType('object');
	expect(worker.id).toBeType('string');
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

	await expect(createWorker({ dtlsCertificateFile: '/foo/cert.pem' }))
		.rejects
		.toThrow(TypeError);

	await expect(createWorker({ dtlsPrivateKeyFile: '/foo/priv.pem' }))
		.rejects
		.toThrow(TypeError);
}, 500);

// test('worker.updateSettings() succeeds', async () =>
// {
// 	const worker = await createWorker();

// 	await expect(worker.updateSettings({ logLevel: 'debug', logTags: [ 'ice' ] }))
// 		.resolves
// 		.toBe(undefined);

// 	worker.close();
// }, 500);
