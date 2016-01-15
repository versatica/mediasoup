'use strict';

const tap = require('tap');

const mediasoup = require('../');

// TODO: waiting for proper parameters definition
tap.test('rtpReceiver.receive() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			let rtpReceiver = peer.RtpReceiver(transport);

			rtpReceiver.receive()
				.then(() =>
				{
					t.pass('rtpReceiver.receive() succeeded');
					t.end();
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

tap.test('rtpReceiver.close() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.plan(3);
	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			let rtpReceiver = peer.RtpReceiver(transport);

			rtpReceiver.on('close', (error) =>
			{
				t.error(error, 'rtpReceiver should close cleanly');

				peer.dump()
					.then((data) =>
					{
						t.pass('peer.dump() succeeded');
						t.equal(Object.keys(data.rtpReceivers).length, 0, 'peer.dump() should retrieve zero rtpReceivers');
					})
					.catch((error) => t.fail(`peer.dump() failed: ${error}`));
			});

			setTimeout(() => rtpReceiver.close(), 100);
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});
