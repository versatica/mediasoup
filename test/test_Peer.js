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
					t.pass('peer.dump() succeeded');
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

tap.test('peer.RtpReceiver() with valid `transport` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			let rtpReceiver = peer.RtpReceiver(transport);

			t.equal(rtpReceiver.transport, transport, 'rtpReceiver.transport must retrieve the given `transport`');
			t.equal(rtpReceiver.rtcpTransport, transport, 'rtpReceiver.rtcpTransport must retrieve the given `transport`');

			peer.dump()
				.then((data) =>
				{
					t.pass('peer.dump() succeeded');
					t.equal(Object.keys(data.rtpReceivers).length, 1, 'peer.dump() should retrieve one rtpReceiver');

					rtpReceiver.on('close', (error) =>
					{
						t.error(error, 'rtpReceiver should close cleanly');
						t.end();
					});

					// Wait a bit so Channel requests receive response
					setTimeout(() => rtpReceiver.close(), 50);
				})
				.catch((error) => t.fail(`peer.dump() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`));
});

tap.test('peer.RtpReceiver() with valid `transport` and `rtcpTransport` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.createAssociatedTransport()
				.then((rtcpTransport) =>
				{
					t.pass('transport.createAssociatedTransport() succeeded');

					let rtpReceiver = peer.RtpReceiver(transport, rtcpTransport);

					t.equal(rtpReceiver.transport, transport, 'rtpReceiver.transport must retrieve the given `transport`');
					t.equal(rtpReceiver.rtcpTransport, rtcpTransport, 'rtpReceiver.rtcpTransport must retrieve the given `rtcpTransport`');
					t.end();
				})
				.catch((error) => t.fail(`transport.createAssociatedTransport() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`));
});

tap.test('peer.RtpReceiver() with a closed `transport` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.close();

			try
			{
				peer.RtpReceiver(transport);

				t.fail('peer.RtpReceiver() succeeded');
			}
			catch (error)
			{
				t.ok(error instanceof mediasoup.errors.InvalidStateError,
					'peer.RtpReceiver() should throw InvalidStateError');
				t.end();
			}
		})
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`));
});
