'use strict';

const tap = require('tap');
const mediasoup = require('../');
const roomOptions = require('./data/options').roomOptions;

tap.test('server.updateSettings() with no options must succeed', { timeout: 2000 }, (t) =>
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

tap.test('server.updateSettings() with valid options must succeed', { timeout: 2000 }, (t) =>
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

tap.test('server.updateSettings() with invalid options must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.updateSettings({ logLevel: 'chicken' })
		.then(() => t.fail('server.updateSettings() succeeded'))
		.catch((error) =>
		{
			t.pass(`server.updateSettings() failed: ${error}`);
			t.end();
		});
});

tap.test('server.updateSettings() in a closed server must fail', { timeout: 2000 }, (t) =>
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
				t.type(error, mediasoup.errors.InvalidStateError, 'server.updateSettings() must reject with InvalidStateError');
				t.end();
			});
	});

	server.close();
});

tap.test('server.createRoom() must succeed', { _timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.createRoom(roomOptions)
		.then((room) =>
		{
			t.pass('server.createRoom() succeeded');

			room.on('close', (error) =>
			{
				t.error(error, 'room must close cleanly');
				t.end();
			});

			// Wait a bit so Channel requests receive response
			setTimeout(() => room.close(), 50);
		})
		.catch((error) => t.fail(`server.createRoom() failed: ${error}`));
});

tap.test('server.createRoom() in a closed server must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	server.close();

	server.createRoom(roomOptions)
		.then((room) => t.fail('server.createRoom() succeeded')) // eslint-disable-line no-unused-vars
		.catch((error) =>
		{
			t.pass(`server.createRoom() failed: ${error}`);
			t.type(error, mediasoup.errors.InvalidStateError, 'server.createRoom() must reject with InvalidStateError');
			t.end();
		});
});

tap.test('server.dump() must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server({ numWorkers: 2 });

	t.tearDown(() => server.close());

	server.dump()
		.then((data) =>
		{
			t.pass('server.dump() succeeded');
			t.equal(Object.keys(data.workers).length, 2, 'server.dump() must retrieve two workers');
			t.end();
		})
		.catch((error) => t.fail(`server.dump() failed: ${error}`));
});
