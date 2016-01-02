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
		.then(() =>
		{
			t.pass('peer.createTransport() succeeded');

			peer.dump()
				.then((data) =>
				{
					t.equal(Object.keys(data.transports).length, 1, 'peer.dump() should retrieve one transport');
					t.end();
				})
				.catch((error) => t.fail(`peer.dump() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`));
});

tap.test('peer.createTransport() with no `udp` nor `tcp` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ udp: false, tcp: false })
		.then(() => t.fail('peer.createTransport() succeeded'))
		.catch(() =>
		{
			t.pass('peer.createTransport() failed');
			t.end();
		});
});
