'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('peer.createTransport() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport()
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`))
		.then(() =>
		{
			t.pass('peer.createTransport() succeeded');

			peer.dump()
				.catch((error) => t.fail(`peer.dump() failed: ${error}`))
				.then((data) =>
				{
					t.pass('peer.dump() succeeded');
					t.equal(Object.keys(data.transports).length, 1, 'peer.dump() should retrieve one transport');
					t.end();
				});
		});
});

tap.test('peer.createTransport() with no `udp` nor `tcp` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ udp: false, tcp: false })
		.catch(() =>
		{
			t.pass('peer.createTransport() failed');
			t.end();
		})
		.then(() => t.fail('peer.createTransport() succeeded'));
});
