'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('room.Peer() with `peerName` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.on('close', (error) =>
	{
		t.error(error, 'peer must close cleanly');
		t.end();
	});

	// Wait a bit so Channel requests receive response
	setTimeout(() => peer.close(), 50);
});

tap.test('room.Peer() without `peerName` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();

	t.throws(() =>
	{
		room.Peer();
	},
	TypeError,
	'room.Peer()) must throw TypeError');

	t.end();
});

tap.test('room.Peer() with same `peerName` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();

	room.Peer('alice');

	t.throws(() =>
	{
		room.Peer('alice');
	},
	'room.Peer() must throw');

	t.end();
});

tap.test('room.Peer() with same `peerName` must succeed if previous peer was closed before', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer1 = room.Peer('alice');

	t.equal(room.getPeer('alice'), peer1, 'room.getPeer() must retrieve the first "alice"');
	t.equal(room.peers.length, 1, 'room.peers must retrieve one peer');

	peer1.close();

	t.notOk(room.getPeer('alice'), 'room.getPeer() must retrieve nothing');
	t.equal(room.peers.length, 0, 'room.peers must retrieve zero peers');

	let peer2 = room.Peer('alice');

	t.equal(room.getPeer('alice'), peer2, 'room.getPeer() must retrieve the new "alice"');
	t.equal(room.peers.length, 1, 'room.peers must retrieve one peer');

	peer2.on('close', (error) =>
	{
		t.error(error, 'peer must close cleanly');
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
			t.equal(Object.keys(data.peers).length, 2, 'room.dump() must retrieve two peers');
			t.end();
		})
		.catch((error) => t.fail(`room.dump() failed: ${error}`));
});
