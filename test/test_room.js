'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('room.createPeer() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.createRoom()
		.then((room) =>
		{
			room.createPeer('alice')
				.then(() => t.end())
				.catch((error) => t.fail(`should not fail: ${error}`));
		})
		.catch((error) => t.fail(`should not fail: ${error}`));
});

tap.test('room.createPeer() without peerId must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.createRoom()
		.then((room) =>
		{
			room.createPeer()
				.then(() => t.fail('should not succeed'))
				.catch(() => t.end());
		})
		.catch((error) => t.fail(`should not fail: ${error}`));
});

tap.test('room.createPeer() with same `peerId` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.createRoom()
		.then((room) =>
		{
			room.createPeer('alice')
				.then(() =>
				{
					room.createPeer('alice')
						.then(() => t.fail('should not succeed'))
						.catch(() => t.end());
				})
				.catch((error) => t.fail(`should not fail: ${error}`));
		})
		.catch((error) => t.fail(`should not fail: ${error}`));
});

tap.test('room.createPeer() with same `peerId` must succeed if previous peer is closed before', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	server.createRoom()
		.then((room) =>
		{
			room.createPeer('alice')
				.then((peer) =>
				{
					let alice1 = peer;

					peer.close();

					room.createPeer('alice')
						.then((peer) =>
						{
							t.notEqual(alice1, peer, 'should be a different "alice"');
							t.equal(room.getPeer('alice'), peer, 'room.getPeer() retrieves the new "alice"');
							t.equal(room.getPeers().length, 1, 'room.getPeers() returns 1 peer');
							t.end();
						})
						.catch((error) => t.fail(`should not fail: ${error}`));
				})
				.catch((error) => t.fail(`should not fail: ${error}`));
		})
		.catch((error) => t.fail(`should not fail: ${error}`));
});
