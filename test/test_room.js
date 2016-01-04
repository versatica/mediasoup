'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('room.Peer() with `peerId` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.on('close', (error) =>
	{
		t.error(error, 'peer should close cleanly');
		t.end();
	});

	// Wait a bit so Channel requests receive response
	setTimeout(() => peer.close(), 50);
});

tap.test('room.Peer() without `peerId` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();

	t.throws(() =>
	{
		room.Peer();
	}, 'room.Peer() should throw error');

	t.end();
});

tap.test('room.Peer() with same `peerId` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();

	room.Peer('alice');

	t.throws(() =>
	{
		room.Peer('alice');
	}, 'room.Peer() should throw error');

	t.end();
});

tap.test('room.Peer() with same `peerId` must succeed if previous peer was closed before', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer1 = room.Peer('alice');

	t.equal(room.getPeer('alice'), peer1, 'room.getPeer() retrieves the first "alice"');
	t.equal(room.getPeers().length, 1, 'room.getPeers() returns one peer');

	peer1.close();

	t.notOk(room.getPeer('alice'), 'room.getPeer() retrieves nothing');
	t.equal(room.getPeers().length, 0, 'room.getPeers() returns zero peers');

	let peer2 = room.Peer('alice');

	t.equal(room.getPeer('alice'), peer2, 'room.getPeer() retrieves the new "alice"');
	t.equal(room.getPeers().length, 1, 'room.getPeers() returns one peer');

	peer2.on('close', (error) =>
	{
		t.error(error, 'peer should close cleanly');
		t.end();
	});

	// Wait a bit so Channel requests receive response
	setTimeout(() => peer2.close(), 50);
});

tap.test('room.dump() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();

	room.Peer('alice');
	room.Peer('bob');

	room.dump()
		.then((data) =>
		{
			t.pass('room.dump() succeeded');
			t.strictSame(Object.keys(data.peers), ['alice', 'bob'], 'room.dump() should retrieve two peers');
			t.end();
		})
		.catch((error) => t.fail(`room.dump() failed: ${error}`));
});
