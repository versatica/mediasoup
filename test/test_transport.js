'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('transport.createAssociatedTransport() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');
			t.equal(transport.iceComponent, 'RTP', 'transport must have "RTP" iceComponent');

			transport.iceLocalCandidates.forEach((candidate) =>
			{
				if (candidate.protocol !== 'udp')
					t.fail('transport contains unexpected "tcp" candidates');
			});
			t.pass('transport just contains "udp" candidates');

			transport.createAssociatedTransport()
				.then((associatedTransport) =>
				{
					t.pass('transport.createAssociatedTransport() succeeded');
					t.equal(associatedTransport.iceComponent, 'RTCP', 'associated transport must have "RTCP" iceComponent');

					associatedTransport.iceLocalCandidates.forEach((candidate) =>
					{
						if (candidate.protocol !== 'udp')
							t.fail('associated transport contains unexpected "tcp" candidates');
					});
					t.pass('associated transport just contains "udp" candidates');

					peer.dump()
						.then((data) =>
						{
							t.pass('peer.dump() succeeded');
							t.equal(Object.keys(data.transports).length, 2, 'peer.dump() should retrieve two transports');
							t.end();
						})
						.catch((error) => t.fail(`peer.dump() failed: ${error}`));
				})
				.catch((error) => t.fail(`transport.createAssociatedTransport() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

tap.test('transport.close() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			transport.on('close', (error) =>
			{
				t.error(error, 'transport should close cleanly');

				peer.dump()
					.then((data) =>
					{
						t.pass('peer.dump() succeeded');
						t.equal(Object.keys(data.transports).length, 0, 'peer.dump() should retrieve zero transports');
						t.end();
					})
					.catch((error) => t.fail(`peer.dump() failed: ${error}`));
			});

			setTimeout(() => transport.close(), 100);
		})
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`));
});
