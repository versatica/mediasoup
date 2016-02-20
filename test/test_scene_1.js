'use strict';

const tap = require('tap');

const mediasoup = require('../');

// TODO: waiting for proper RTP parameters definition
tap.test('alice and bob create RtpReceivers and expect RtpSenders', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server({ numWorkers: 1 });

	t.tearDown(() => server.close());

	let room = server.Room();

	t.pass('room created');

	let alice = room.Peer('alice');

	t.pass('alice created');

	let bob = room.Peer('bob');

	t.pass('bob created');

	let aliceAudioParameters =
	{
		muxId  : 'alicemuxid1',
		codecs :
		[
			{
				name        : 'opus',
				kind        : 'audio',
				payloadType : 101,
				clockRate   : null
			}
		],
		encodings :
		[
			{
				ssrc : 100000001
			}
		]
	};
	let aliceVideoParameters =
	{
		muxId  : 'alicemuxid2',
		codecs :
		[
			{
				name        : 'vp9',
				kind        : 'video',
				payloadType : 102,
				clockRate   : null
			}
		],
		encodings :
		[
			{
				ssrc : 100000002
			}
		]
	};
	let bobVideoParameters =
	{
		muxId  : 'bobmuxid2',
		codecs :
		[
			{
				name        : 'vp9',
				kind        : 'video',
				payloadType : 202,
				clockRate   : null
			},
			{
				name        : 'vp8',
				kind        : 'video',
				payloadType : 203,
				clockRate   : null
			}
		],
		encodings :
		[
			{
				ssrc : 200000002
			}
		]
	};

	Promise.all(
		[
			new Promise((accept, reject) =>
			{
				let numEvents = 0;

				alice.on('newrtpsender', (rtpSender, receiverPeer) =>
				{
					t.pass('alice "newrtpsender" event fired');
					t.equal(receiverPeer, bob, '`receiverPeer` must be bob');

					let transport = alice.getTransports()[0];

					rtpSender.setTransport(transport)
						.then(() =>
						{
							t.pass('rtpSender.setTransport() succeeded');
							t.equal(rtpSender.transport, transport, 'rtpSender.transport must retrieve the given `transport`');

							// We expect a single event
							if (++numEvents === 1)
								accept();
						})
						.catch((error) =>
						{
							t.fail(`rtpSender.setTransport() failed: ${error}`);
							reject();
						});
				});
			}),

			new Promise((accept, reject) =>
			{
				let numEvents = 0;

				bob.on('newrtpsender', (rtpSender, receiverPeer) =>
				{
					t.pass('bob "newrtpsender" event fired');
					t.equal(receiverPeer, alice, '`receiverPeer` must be alice');

					let transport = bob.getTransports()[0];

					rtpSender.setTransport(transport)
						.then(() =>
						{
							t.pass('rtpSender.setTransport() succeeded');
							t.equal(rtpSender.transport, transport, 'rtpSender.transport must retrieve the given `transport`');

							// We expect two events
							if (++numEvents === 2)
								accept();
						})
						.catch((error) =>
						{
							t.fail(`rtpSender.setTransport() failed: ${error}`);
							reject();
						});
				});
			}),

			createTransportAndRtpReceivers(t, alice, [ aliceAudioParameters, aliceVideoParameters ]),

			createTransportAndRtpReceivers(t, bob, [ bobVideoParameters ])
		])
		.then(() =>
		{
			t.equal(alice.getRtpSenders().length, 1, 'alice.getRtpSenders() must retrieve 1');
			t.equal(bob.getRtpSenders().length, 2, 'bob.getRtpSenders() must retrieve 2');

			t.same(alice.getRtpSenders()[0].rtpParameters, bobVideoParameters,
				'first RtpSender parameters of alice must match video parameters of bob');
			t.same(bob.getRtpSenders()[0].rtpParameters, aliceAudioParameters,
				'first RtpSender parameters of bob must match audio parameters of alice');
			t.same(bob.getRtpSenders()[1].rtpParameters, aliceVideoParameters,
				'second RtpSender parameters of bob must match video parameters of alice');
		})
		.then(() =>
		{
			let aliceAudioReceiver = alice.getRtpReceivers()[0];
			let aliceVideoReceiver = alice.getRtpReceivers()[1];
			let aliceVideoSender = alice.getRtpSenders()[0];
			let bobVideoReceiver = bob.getRtpReceivers()[0];
			let bobAudioSender = bob.getRtpSenders()[0];
			let bobVideoSender = bob.getRtpSenders()[1];

			return room.dump()
				.then((data) =>
				{
					t.pass('room.dump() succeeded');

					t.same(data.mapRtpReceiverRtpSenders,
						{
							[aliceAudioReceiver._internal.rtpReceiverId] :
								[
									String(bobAudioSender._internal.rtpSenderId)
								],
							[aliceVideoReceiver._internal.rtpReceiverId] :
								[
									String(bobVideoSender._internal.rtpSenderId)
								],
							[bobVideoReceiver._internal.rtpReceiverId] :
								[
									String(aliceVideoSender._internal.rtpSenderId)
								]
						},
						'`data.mapRtpReceiverRtpSenders` match the expected values');
				})
				.catch((error) => t.fail(`room.dump() failed: ${error}`));
		})
		.then(() =>
		{
			return t.test('close aliceAudioReceiver', { timeout: 1000 }, (t2) =>
			{
				t2.plan(2);

				let aliceAudioReceiver = alice.getRtpReceivers()[0];
				let bobAudioSender = bob.getRtpSenders()[0];

				aliceAudioReceiver.on('close', () => t2.pass('aliceAudioReceiver closed'));
				bobAudioSender.on('close', () => t2.pass('bobAudioSender closed'));

				aliceAudioReceiver.close();
			});
		})
		.then(() =>
		{
			let aliceVideoReceiver = alice.getRtpReceivers()[0];
			let aliceVideoSender = alice.getRtpSenders()[0];
			let bobVideoReceiver = bob.getRtpReceivers()[0];
			let bobVideoSender = bob.getRtpSenders()[0];

			return room.dump()
				.then((data) =>
				{
					t.pass('room.dump() succeeded');

					t.same(data.mapRtpReceiverRtpSenders,
						{
							[aliceVideoReceiver._internal.rtpReceiverId] :
								[
									String(bobVideoSender._internal.rtpSenderId)
								],
							[bobVideoReceiver._internal.rtpReceiverId] :
								[
									String(aliceVideoSender._internal.rtpSenderId)
								]
						},
						'`data.mapRtpReceiverRtpSenders` match the expected values');
				})
				.catch((error) => t.fail(`room.dump() failed: ${error}`));
		})
		.then(() =>
		{
			return t.test('close bob', { timeout: 1000 }, (t2) =>
			{
				t2.plan(4);

				let aliceVideoReceiver = alice.getRtpReceivers()[0];
				let aliceVideoSender = alice.getRtpSenders()[0];
				let bobVideoReceiver = bob.getRtpReceivers()[0];
				let bobVideoSender = bob.getRtpSenders()[0];

				bob.on('close', () => t2.pass('bob closed'));
				bobVideoReceiver.on('close', () => t2.pass('bobVideoReceiver closed'));
				bobVideoSender.on('close', () => t2.pass('bobVideoSender closed'));
				aliceVideoSender.on('close', () => t2.pass('aliceVideoSender closed'));

				bob.close();
			});
		})
		.then(() =>
		{
			t.equal(alice.getRtpReceivers().length, 1, 'alice.getRtpReceivers() must retrieve 1');
			t.equal(bob.getRtpReceivers().length, 0, 'bob.getRtpReceivers() must retrieve 0');
			t.equal(alice.getRtpSenders().length, 0, 'alice.getRtpSenders() must retrieve 0');
			t.equal(bob.getRtpSenders().length, 0, 'bob.getRtpSenders() must retrieve 0');

			let aliceVideoReceiver = alice.getRtpReceivers()[0];

			return room.dump()
				.then((data) =>
				{
					t.pass('room.dump() succeeded');

					t.same(data.mapRtpReceiverRtpSenders,
						{
							[aliceVideoReceiver._internal.rtpReceiverId] : []
						},
						'`data.mapRtpReceiverRtpSenders` match the expected values');

					t.end();
				})
				.catch((error) => t.fail(`room.dump() failed: ${error}`));
		});
});

function createTransportAndRtpReceivers(t, peer, rtpParametersList)
{
	return peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass(`transport created for ${peer.name}`);

			let promises = [];

			for (let rtpParameters of rtpParametersList)
			{
				let promise = createRtpReceiver(t, peer, transport, rtpParameters);

				promises.push(promise);
			}

			return Promise.all(promises);
		})
		.catch((error) => t.fail(`peer.createTransport() failed for ${peer.name}: ${error}`));
}

function createRtpReceiver(t, peer, transport, rtpParameters)
{
	let rtpReceiver = peer.RtpReceiver(transport);

	return rtpReceiver.receive(rtpParameters)
		.then(() =>
		{
			t.pass(`rtpReceiver.receive() succeeded for ${peer.name}`);
		})
		.catch((error) => t.fail(`rtpReceiver.receive() failed for ${peer.name}: ${error}`));
}
