'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('mediasoup.createServer() with no options must succeed', { timeout: 500 }, (t) =>
{
	let server = mediasoup.createServer();

	server.on('close', (error) =>
	{
		t.error(error, 'should close gracefully');
		t.end();
	});

	setTimeout(() => server.close(), 100);
});

tap.test('mediasoup.createServer() with no options must succeed', { timeout: 500 }, (t) =>
{
	let server = mediasoup.createServer({ logLevel: 'WRONG_VALUE' });

	server.on('close', (error) =>
	{
		t.ok(error, 'should close with error');
		t.end();
	});
});
