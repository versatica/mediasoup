'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('mediasoup.createServer() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.createServer();

	server.on('close', (error) =>
	{
		if (!error)
			t.end();
		else
			t.fail('should not be closed with error');
	});

	setTimeout(() => server.close(), 100);
});

tap.test('mediasoup.createServer() with valid options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.createServer(
		{
			numWorkers : 1,
			logLevel   : 'warn'
		});

	server.on('close', (error) =>
	{
		if (!error)
			t.end();
		else
			t.fail('should not be closed with error');
	});

	setTimeout(() => server.close(), 100);
});

tap.test('mediasoup.createServer() with wrong options must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.createServer({ logLevel: 'WRONG_VALUE' });

	server.on('close', (error) =>
	{
		if (error)
			t.end();
		else
			t.fail('should be closed with error');
	});
});
