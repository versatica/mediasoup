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

					room.dump()
						.then((data) =>
						{
							t.equal(Object.keys(data.peers).length, 0, 'room.dump() should retrieve zero peers');
						})
						.catch((error) => t.fail(`should not fail: ${error}`));

					room.createPeer('alice')
						.then((peer) =>
						{
							t.notEqual(alice1, peer, 'new peer should be a different "alice"');
							t.equal(room.getPeer('alice'), peer, 'room.getPeer() retrieves the new "alice"');
							t.equal(room.getPeers().length, 1, 'room.getPeers() returns one peer');
							t.end();
						})
						.catch((error) => t.fail(`should not fail: ${error}`));

					room.dump()
						.then((data) =>
						{
							t.equal(Object.keys(data.peers).length, 1, 'room.dump() should retrieve one peer');
							t.equal(Object.keys(data.peers)[0], 'alice', 'room.dump() should retrieve ane peer with key "alice"');
						})
						.catch((error) => t.fail(`should not fail: ${error}`));
				})
				.catch((error) => t.fail(`should not fail: ${error}`));
		})
		.catch((error) => t.fail(`should not fail: ${error}`));
});
