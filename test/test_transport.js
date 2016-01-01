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
			t.equal(transport.iceComponent, 'RTP', 'transport must have "RTP" iceComponent');

			transport.iceLocalCandidates.forEach((candidate) =>
			{
				t.equal(candidate.protocol, 'udp', 'there should be just "udp" candidates');
			});

			transport.createAssociatedTransport()
				.then((associatedTransport) =>
				{
					t.equal(associatedTransport.iceComponent, 'RTCP', 'associated transport must have "RTCP" iceComponent');

					associatedTransport.iceLocalCandidates.forEach((candidate) =>
					{
						t.equal(candidate.protocol, 'udp', 'there should be just "udp" candidates');
					});

					peer.dump()
						.then((data) =>
						{
							t.equal(Object.keys(data.transports).length, 2, 'peer.dump() should retrieve two transports');
							t.end();
						})
						.catch((error) => t.fail(`should not fail: ${error}`));
				})
				.catch((error) => t.fail(`should not fail: ${error}`));
		})
		.catch((error) => t.fail(`should not fail: ${error}`));
});
