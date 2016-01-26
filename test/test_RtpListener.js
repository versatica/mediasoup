'use strict';

const tap = require('tap');

const mediasoup = require('../');

// TODO: waiting for proper parameters definition
tap.test('lalala', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			let rtpReceiver1 = peer.RtpReceiver(transport);
			let rtpParameters1 =
			{
				muxId  : 'abcd',
				codecs :
				[
					{
						name        : 'opus',
						kind        : 'audio',
						payloadType : 111
					}
				],
				encodings :
				[
					{
						ssrc : 111222330
					},
					{
						ssrc : 111222331
					}
				]
			};
			let rtpReceiver2 = peer.RtpReceiver(transport);
			let rtpParameters2 =
			{
				muxId  : '1234',
				codecs :
				[
					{
						name        : 'PCMU',
						kind        : 'audio',
						payloadType : 0
					}
				],
				encodings :
				[
					{
						ssrc : 987654321
					}
				]
			};

			Promise.all(
				[
					rtpReceiver1.receive(rtpParameters1)
						.then(() =>
						{
							t.pass('first rtpReceiver.receive() succeeded');
						})
						.catch((error) => t.fail(`first rtpReceiver.receive() failed: ${error}`)),

					rtpReceiver2.receive(rtpParameters2)
						.then(() =>
						{
							t.pass('second rtpReceiver.receive() succeeded');
						})
						.catch((error) => t.fail(`second rtpReceiver.receive() failed: ${error}`))
				]).then(() =>
				{
					t.pass('both rtpReceiver.receive() succeeded');

					transport.dump()
						.then((data) =>
						{
							t.pass('transport.dump() succeeded');
							t.same(Object.keys(data.rtpListener.ptTable).sort(), [ '111', '0' ].sort(), 'transport.dump() must provide the given payload types');
							t.same(Object.keys(data.rtpListener.ssrcTable).sort(), [ '111222330', '111222331', '987654321' ].sort(), 'transport.dump() must provide the given ssrc values');
							t.end();
						})
						.catch((error) => t.fail(`transport.dump() failed: ${error}`));
				});
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

