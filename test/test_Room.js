'use strict';

const tap = require('tap');

const mediasoup = require('../');
const roomOptions = require('./data/options').roomOptions;
const peerOptions = require('./data/options').peerOptions;

tap.test('room.Peer() with peerName must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);
	let peer = room.Peer('alice', peerOptions);

	peer.on('close', (error) =>
	{
		t.error(error, 'peer must close cleanly');
		t.end();
	});

	// Wait a bit so Channel requests receive response
	setTimeout(() => peer.close(), 50);
});

tap.test('room.Peer() without peerName must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);

	t.throws(() =>
	{
		room.Peer();
	},
	TypeError,
	'room.Peer()) must throw TypeError');

	t.end();
});

tap.test('room.Peer() with same peerName must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);

	room.Peer('alice', peerOptions);

	t.throws(() =>
	{
		room.Peer('alice', peerOptions);
	},
	'room.Peer() must throw');

	t.end();
});

tap.test('room.Peer() with same peerName must succeed if previous peer was closed before', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);
	let peer1 = room.Peer('alice', peerOptions);

	t.equal(room.getPeer('alice'), peer1, 'room.getPeer() must retrieve the first "alice"');
	t.equal(room.peers.length, 1, 'room.peers must retrieve one peer');

	peer1.close();

	t.notOk(room.getPeer('alice'), 'room.getPeer() must retrieve nothing');
	t.equal(room.peers.length, 0, 'room.peers must retrieve zero peers');

	let peer2 = room.Peer('alice', peerOptions);

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

tap.test('room.peers must retrieve existing peers', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);

	let alice = room.Peer('alice', peerOptions);
	let bob = room.Peer('bob', peerOptions);
	let carol = room.Peer('carol', peerOptions);

	bob.close();

	t.same(room.peers, [ alice, carol ], 'room.peers() must retrieve "alice" and "carol" peers');

	t.end();
});

tap.test('room.getCapabilities() must retrieve current room capabilities', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);

	room.getCapabilities()
		.then((capabilities) =>
		{
			t.pass('room.getCapabilities() succeeded');
			t.end();
		})
		.catch((error) => t.fail(`room.getCapabilities() failed: ${error}`));
});

tap.test('room.dump() must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);

	room.Peer('alice', peerOptions);
	room.Peer('bob', peerOptions);

	room.dump()
		.then((data) =>
		{
			t.pass('room.dump() succeeded');
			t.equal(Object.keys(data.peers).length, 2, 'room.dump() must retrieve two peers');
			t.end();
		})
		.catch((error) => t.fail(`room.dump() failed: ${error}`));
});
