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
				kind   : 'audio',
				muxId  : 'abcd',
				codecs :
				[
					{
						name        : 'opus',
						payloadType : 111,
						numChannels : 2
					},
					{
						name        : 'PCMA',
						payloadType : 8,
						clockRate   : 4800,
						maxptime    : 20
					},
					{
						name         : 'VP8',
						payloadType  : 103,
						clockRate    : 90000,
						rtcpFeedback :
						[
							{ type: 'ccm',         parameter: 'fir' },
							{ type: 'nack',        parameter: ''    },
							{ type: 'nack',        parameter: 'pli' },
							{ type: 'google-remb', parameter: ''    }
						]
					}
				],
				encodings :
				[
					{
						ssrc             : 111222330,
						codecPayloadType : 111,
						fec :
						{
							mechanism : 'foo',
							ssrc      : 222222222
						},
						rtx :
						{
							payloadType : 201,
							ssrc        : 333333333
						}
					},
					{
						ssrc             : 111222331,
						codecPayloadType : 8
					}
				],
				rtcp :
				{
					cname       : 'a7sdihkj3sdsdflqwkejl98ujk',
					ssrc        : 88888888,
					reducedSize : true
				}
			};

			t.equal(rtpReceiver.transport, transport, 'rtpReceiver.transport must retrieve the given `transport`');

			rtpReceiver.receive(rtpParameters)
				.then(() =>
				{
					t.pass('rtpReceiver.receive() succeeded');

					return Promise.all(
						[
							transport.dump()
								.then((data) =>
								{
									t.pass('transport.dump() succeeded');
									t.same(Object.keys(data.rtpListener.ssrcTable).sort(), [ '111222330', '111222331' ].sort(), 'transport.dump() must provide the given ssrc values');
									t.same(Object.keys(data.rtpListener.ptTable).sort(), [ '111', '8', '103' ].sort(), 'transport.dump() must provide the given payload types');
								})
								.catch((error) => t.fail(`transport.dump() failed: ${error}`)),

							rtpReceiver.dump()
								.then((data) =>
								{
									t.pass('rtpReceiver.dump() succeeded');
									t.same(data.rtpParameters, rtpParameters, 'rtpReceiver.dump() must provide the expected `rtpParameters`');
								})
								.catch((error) => t.fail(`rtpReceiver.dump() failed: ${error}`))
						])
							.then(() => t.end());
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`));
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
