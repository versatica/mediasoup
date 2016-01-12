'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('server.updateSettings() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.updateSettings()
		.then(() =>
		{
			t.pass('server.updateSettings() succeeded');
			t.end();
		})
		.catch((error) => t.fail(`server.updateSettings() failed: ${error}`));
});

tap.test('server.updateSettings() with valid options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.updateSettings({ logLevel: 'warn' })
		.then(() =>
		{
			t.pass('server.updateSettings() succeeded');
			t.end();
		})
		.catch((error) => t.fail(`server.updateSettings() failed: ${error}`));
});

tap.test('server.updateSettings() with invalid options must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.updateSettings({ logLevel: 'WRONG_LOG_LEVEL' })
		.then(() => t.fail('server.updateSettings() succeeded'))
		.catch((error) =>
		{
			t.pass(`server.updateSettings() failed: ${error}`);
			t.end();
		});
});

tap.test('server.updateSettings() in a closed server must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.on('close', () =>
	{
		server.updateSettings({ logLevel: 'error' })
			.then(() => t.fail('server.updateSettings() succeeded'))
			.catch((error) =>
			{
				t.pass(`server.updateSettings() failed: ${error}`);
				t.type(error, mediasoup.errors.InvalidStateError, 'server.updateSettings() error must be InvalidStateError');
				t.end();
			});
	});

	server.close();
});

tap.test('server.Room() must succeed', { _timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();

	room.on('close', (error) =>
	{
		t.error(error, 'room should close cleanly');
		t.end();
	});

	// Wait a bit so Channel requests receive response
	setTimeout(() => room.close(), 50);
});

tap.test('server.Room() in a closed server must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.on('close', () =>
	{
		try
		{
			server.Room();

			t.fail('server.Room() succeeded');
		}
		catch (error)
		{
			t.ok(error instanceof mediasoup.errors.InvalidStateError,
				'server.Room() should throw InvalidStateError');
			t.end();
		}
	});

	server.close();
});

tap.test('server.dump() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server({ numWorkers: 2 });

	t.tearDown(() => server.close());

	server.dump()
		.then((data) =>
		{
			t.pass('server.dump() succeeded');
			t.equal(Object.keys(data.workers).length, 2, 'server.dump() should retrieve two workers');
			t.end();
		})
		.catch((error) => t.fail(`server.dump() failed: ${error}`));
});
