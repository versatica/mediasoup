'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('peer.createTransport() with no options must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let pRoom = server.createRoom();
	pRoom.catch((error) => t.fail(`should not fail: ${error}`));

	let pPeer = pRoom.then((room) => room.createPeer('alice'));
	pPeer.catch((error) => t.fail(`should not fail: ${error}`));

	pPeer.then((peer) =>
	{
		peer.createTransport()
			.catch((error) => t.fail(`should not fail: ${error}`));
	});

	pPeer.then((peer) =>
	{
		peer.dump()
			.then((data) =>
			{
				t.equal(Object.keys(data.transports).length, 1, 'peer.dump() should retrieve one transport');
				t.end();
			})
			.catch((error) => t.fail(`should not fail: ${error}`));
	});
});
