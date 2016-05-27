'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('mediasoup.Server() with no options must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	server.on('close', (error) =>
	{
		t.error(error, 'server must close cleanly');
		t.end();
	});

	setTimeout(() => server.close(), 100);
});

tap.test('mediasoup.Server() with valid options must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server(
		{
			numWorkers : 1,
			logLevel   : 'warn'
		});

	server.on('close', (error) =>
	{
		t.error(error, 'server must close cleanly');
		t.end();
	});

	setTimeout(() => server.close(), 100);
});

tap.test('mediasoup.Server() with wrong options must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server({ logLevel: 'WRONG_VALUE' });

	server.on('close', (error) =>
	{
		t.type(error, Error, 'server must close with error');
		t.end();
	});
});

tap.test('mediasoup.Server() with non existing rtcListenIPv4 IP must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server({ rtcListenIPv4: '1.2.3.4' });

	server.on('close', (error) =>
	{
		t.type(error, Error, 'server must close with error');
		t.end();
	});
});

tap.test('mediasoup.Server() with too narrow RTC ports range must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server({ rtcMinPort: 2000, rtcMaxPort: 2050 });

	server.on('close', (error) =>
	{
		t.type(error, Error, 'server must close with error');
		t.end();
	});
});
