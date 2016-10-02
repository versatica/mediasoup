'use strict';

const tap = require('tap');
const mediasoup = require('../');
const roomOptions = require('./data/options').roomOptions;
const peerCapabilities = require('./data/options').peerCapabilities;

function initTest(t)
{
	let server = mediasoup.Server();
	let peer;

	t.tearDown(() => server.close());

	return server.createRoom(roomOptions)
		.then((room) =>
		{
			peer = room.Peer('alice');

			return peer.setCapabilities(peerCapabilities);
		})
		.then(() =>
		{
			return { peer: peer };
		});
}

tap.test('peer.createTransport() with no options must succeed', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;

			t.equal(peer.name, 'alice', 'peer.name must be "alice"');

			return peer.createTransport()
				.then(() =>
				{
					t.pass('peer.createTransport() succeeded');

					return peer.dump()
						.then((data) =>
						{
							t.pass('peer.dump() succeeded');
							t.equal(Object.keys(data.transports).length, 1, 'peer.dump() must retrieve one transport');
							t.end();
						})
						.catch((error) => t.fail(`peer.dump() failed: ${error}`));
				});
		});
});

tap.test('peer.createTransport() with no udp nor tcp must fail', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;

			return peer.createTransport({ udp: false, tcp: false })
				.then(() => t.fail('peer.createTransport() succeeded'))
				.catch((error) =>
				{
					t.pass(`peer.createTransport() failed: ${error}`);
				});
		});
});

tap.test('peer.RtpReceiver() with valid transport must succeed', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;
			let rtpReceiver;

			return peer.createTransport({ tcp: false })
				.then((transport) =>
				{
					t.pass('peer.createTransport() succeeded');

					rtpReceiver = peer.RtpReceiver('audio', transport);
					t.pass('peer.RtpReceiver() succeeded');

					t.equal(rtpReceiver.transport, transport, 'rtpReceiver.transport must retrieve the given transport');

					return peer.dump();
				})
				.then((data) =>
				{
					t.pass('peer.dump() succeeded');
					t.equal(Object.keys(data.rtpReceivers).length, 1, 'peer.dump() must retrieve one rtpReceiver');
					t.same(peer.rtpReceivers[0], rtpReceiver, 'peer.rtpReceivers[0] must retrieve the previous rtpReceiver');
				});
		});
});

tap.test('peer.RtpReceiver() with a closed transport must fail', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;

			return peer.createTransport({ tcp: false })
				.then((transport) =>
				{
					t.pass('peer.createTransport() succeeded');

					transport.close();

					t.throws(() =>
					{
						peer.RtpReceiver('audio', transport);
					},
					mediasoup.errors.InvalidStateError,
					'peer.RtpReceiver() must throw InvalidStateError');
				});
		});
});

tap.test('peer.RtpReceiver() with invalid kind must fail', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;

			return peer.createTransport({ tcp: false })
				.then((transport) =>
				{
					t.pass('peer.createTransport() succeeded');

					t.throws(() =>
					{
						peer.RtpReceiver('chicken', transport);
					},
					TypeError,
					'peer.RtpReceiver() must throw TypeError');
				});
		});
});
