'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('server.updateSettings() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.updateSettings()
		.then(() => t.end())
		.catch((error) => t.fail(`should not fail: ${error}`));
});

tap.test('server.updateSettings() with valid options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.updateSettings({ logLevel: 'warn' })
		.then(() => t.end())
		.catch((error) => t.fail(`should not fail: ${error}`));
});

tap.test('server.updateSettings() with invalid options must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.updateSettings({ logLevel: 'WRONG_LOG_LEVEL' })
		.then(() => t.fail('should not succeed'))
		.catch(() => t.end());
});

tap.test('server.updateSettings() in a closed Server must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	server.on('close', () =>
	{
		server.updateSettings({ logLevel: 'error' })
			.then(() => t.fail('should not succeed'))
			.catch(() => t.end());
	});

	server.close();
});

tap.test('server.createRoom() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.createRoom()
		.then(() => t.end())
		.catch((error) => t.fail(`should not fail: ${error}`));
});

tap.test('server.createRoom() in a closed Server must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	server.on('close', () =>
	{
		server.createRoom()
			.then(() => t.fail('should not succeed'))
			.catch(() => t.end());
	});

	server.close();
});

tap.test('server.dump() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server({ numWorkers: 1 });

	server.on('close', () =>
	{
		server.createRoom()
			.then(() => t.fail('should not succeed'))
			.catch(() => t.end());
	});

	server.dump()
		.then((data) =>
		{
			t.equal(Object.keys(data.workers).length, 1, 'server.dump() should retrieve one worker');
			server.close();
		})
		.catch((error) => t.fail(`should not fail: ${error}`));
});
