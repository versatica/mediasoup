'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('server.updateSettings() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.createServer();

	t.tearDown(() => server.close());

	server.updateSettings()
		.then(()  => t.end())
		.catch(() => t.fail('should not fail'));
});

tap.test('server.updateSettings() with valid options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.createServer();

	t.tearDown(() => server.close());

	server.updateSettings({ logLevel: 'warn' })
		.then(()  => t.end())
		.catch(() => t.fail('should not fail'));
});
