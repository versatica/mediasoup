'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('mediasoup.Server() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	server.on('close', (error) =>
	{
		if (!error)
			t.end();
		else
			t.fail('should not be closed with error');
	});

	setTimeout(() => server.close(), 100);
});

tap.test('mediasoup.Server() with valid options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server(
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

tap.test('mediasoup.Server() with wrong options must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server({ logLevel: 'WRONG_VALUE' });

	server.on('close', (error) =>
	{
		if (error)
			t.end();
		else
			t.fail('should be closed with error');
	});
});
