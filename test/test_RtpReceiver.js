'use strict';

const tap = require('tap');

const mediasoup = require('../');

// TODO: waiting for proper parameters definition
tap.test('rtpReceiver.receive() with valid `rtpParameters` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			let rtpReceiver = peer.RtpReceiver(transport);
			let rtpParameters =
			{
				muxId  : 'abcd',
				codecs :
				[
					{
						name        : 'opus',
						kind        : 'audio',
						payloadType : 101
					},
					{
						name        : 'vp9',
						kind        : 'video',
						payloadType : 102,
						clockRate   : 50
					},
					{
						name        : 'supercodec',
						payloadType : 103
					}
				],
				encodings :
				[
				]
			};
			let expectedRtpParameters =
			{
				muxId  : 'abcd',
				codecs :
				[
					{
						name        : 'opus',
						kind        : 'audio',
						payloadType : 101,
						clockRate   : null
					},
					{
						name        : 'vp9',
						kind        : 'video',
						payloadType : 102,
						clockRate   : 50
					},
					{
						name        : 'supercodec',
						kind        : '',
						payloadType : 103,
						clockRate   : null
					}
				],
				encodings :
				[
				]
			};

			rtpReceiver.receive(rtpParameters)
				.then(() =>
				{
					t.pass('rtpReceiver.receive() succeeded');

					rtpReceiver.dump()
						.then((data) =>
						{
							t.pass('rtpReceiver.dump() succeeded');
							t.same(data.rtpParameters, expectedRtpParameters, 'rtpReceiver.dump() must provide the expected `rtpParameters`');
							t.end();
						})
						.catch((error) => t.fail(`rtpReceiver.dump() failed: ${error}`));
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

tap.test('rtpReceiver.receive() with no `rtpParameters` must fail', { timeout: 1000 }, (t) =>
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
				.then(() => t.fail('rtpReceiver.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver.receive() failed: ${error}`);
					t.end();
				});
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
				t.error(error, 'rtpReceiver must close cleanly');

				peer.dump()
					.then((data) =>
					{
						t.pass('peer.dump() succeeded');
						t.equal(Object.keys(data.rtpReceivers).length, 0, 'peer.dump() must retrieve zero rtpReceivers');
					})
					.catch((error) => t.fail(`peer.dump() failed: ${error}`));
			});

			setTimeout(() => rtpReceiver.close(), 100);
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});
