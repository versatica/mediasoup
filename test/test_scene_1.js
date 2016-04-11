'use strict';

const tap = require('tap');

const mediasoup = require('../');

// TODO: waiting for proper RTP parameters definition
tap.test('alice, bob and carol create RtpReceivers and expect RtpSenders', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server({ numWorkers: 1 });

	t.tearDown(() => server.close());

	let room = server.Room();

	t.pass('room created');

	let alice = room.Peer('alice');

	t.pass('alice created');

	let bob = room.Peer('bob');

	t.pass('bob created');

	let carol;

	let aliceAudioParameters =
	{
		kind   : 'audio',
		muxId  : 'alice-audio',
		codecs :
		[
			{
				name        : 'opus',
				payloadType : 101,
				clockRate   : 90000
			}
		],
		encodings :
		[
			{
				ssrc : 100000011
			}
		],
		rtcp :
		{
			cname       : 'ALICEAUDIO',
			ssrc        : 51000011,
			reducedSize : false
		}
	};
	let aliceVideoParameters =
	{
		kind   : 'video',
		muxId  : 'alice-video',
		codecs :
		[
			{
				name        : 'vp9',
				payloadType : 102,
				clockRate   : 90000,
				rtx :
				{
					payloadType : 96,
					rtxTime     : 1000
				},
				fec :
				[
					{ mechanism: 'foo', payloadType: 97 }
				]
			}
		],
		encodings :
		[
			{
				ssrc    : 100000021,
				rtxSsrc : 100000022,
				fecSsrc : 100000023
			}
		],
		rtcp :
		{
			cname       : 'ALICEVIDEO',
			ssrc        : 51000021,
			reducedSize : true
		}
	};
	let bobVideoParameters =
	{
		kind   : 'video',
		muxId  : 'bob-video',
		codecs :
		[
			{
				name        : 'vp9',
				payloadType : 103,
				clockRate   : 90000,
				rtx :
				{
					payloadType : 97,
					rtxTime     : 2000
				}
			}
		],
		encodings :
		[
			{
				ssrc    : 200000021,
				rtxSsrc : 200000022
			}
		],
		rtcp :
		{
			cname       : 'BOBVIDEO',
			ssrc        : 52000021,
			reducedSize : true
		}
	};
	let carolAudioParameters =
	{
		kind   : 'audio',
		muxId  : 'carol-audio',
		codecs :
		[
			{
				name        : 'opus',
				payloadType : 103,
				clockRate   : 90000,
				fec :
				[
					{ mechanism: 'bar', payloadType: 98 }
				]
			}
		],
		encodings :
		[
			{
				ssrc    : 300000011,
				fecSsrc : 300000012
			}
		],
		rtcp :
		{
			cname       : 'CAROLAUDIO',
			ssrc        : 53000011,
			reducedSize : true
		}
	};
	let carolVideoParameters =
	{
		kind   : 'video',
		muxId  : 'carol-video',
		codecs :
		[
			{
				name        : 'vp9',
				payloadType : 103,
				rtx :
				{
					payloadType : 97,
					rtxTime     : 1500
				}
			}
		],
		encodings :
		[
			{
				ssrc    : 300000021,
				rtxSsrc : 300000022
			}
		],
		rtcp :
		{
			cname       : 'CAROLVIDEO',
			ssrc        : 53000021,
			reducedSize : true
		}
	};

	Promise.all(
		[
			createTransportAndRtpReceivers(t, alice, [ aliceAudioParameters, aliceVideoParameters ])
				// Once transport is created let's check the already existing RtpSenders
				// and listen for 'newrtpsender' if all the expected ones are not yet
				// present
				.then(() =>
				{
					const EXPECTED_RTPSENDERS = 1;

					let handledRtpSenders = 0;
					let availableRtpSenders = alice.rtpSenders;

					let promise1 = new Promise((accept, reject) =>
					{
						if (availableRtpSenders.length === 0)
						{
							accept();
							return;
						}

						for (let rtpSender of availableRtpSenders)
						{
							handleRtpSender(rtpSender);
						}

						function handleRtpSender(rtpSender)
						{
							t.pass('rtpSender retrieved via alice.rtpSenders');
							t.equal(rtpSender.associatedPeer, bob, '`receiverPeer` must be bob');

							let transport = alice.transports[0];

							rtpSender.setTransport(transport)
								.then(() =>
								{
									t.pass('rtpSender.setTransport() succeeded');
									t.equal(rtpSender.transport, transport, 'rtpSender.transport must retrieve the given `transport`');

									if (++handledRtpSenders === EXPECTED_RTPSENDERS)
										accept();
								})
								.catch((error) =>
								{
									t.fail(`rtpSender.setTransport() failed: ${error}`);
									reject(error);
								});
						}
					});

					let promise2 = new Promise((accept, reject) =>
					{
						if (availableRtpSenders.length === EXPECTED_RTPSENDERS)
						{
							accept();
							return;
						}

						alice.on('newrtpsender', (rtpSender) =>
						{
							t.pass('alice "newrtpsender" event fired');
							t.equal(rtpSender.associatedPeer, bob, '`rtpSender.associatedPeer` must be bob');

							let transport = alice.transports[0];

							rtpSender.setTransport(transport)
								.then(() =>
								{
									t.pass('rtpSender.setTransport() succeeded');
									t.equal(rtpSender.transport, transport, 'rtpSender.transport must retrieve the given `transport`');

									if (++handledRtpSenders === EXPECTED_RTPSENDERS)
										accept();
								})
								.catch((error) =>
								{
									t.fail(`rtpSender.setTransport() failed: ${error}`);
									reject(error);
								});
						});
					});

					return Promise.all([ promise1, promise2 ]);
				}),

			createTransportAndRtpReceivers(t, bob, [ bobVideoParameters ])
				.then(() =>
				{
					const EXPECTED_RTPSENDERS = 2;

					let handledRtpSenders = 0;
					let availableRtpSenders = bob.rtpSenders;

					let promise1 = new Promise((accept, reject) =>
					{
						if (availableRtpSenders.length === 0)
						{
							accept();
							return;
						}

						for (let rtpSender of availableRtpSenders)
						{
							handleRtpSender(rtpSender);
						}

						function handleRtpSender(rtpSender)
						{
							t.pass('rtpSender retrieved via bob.rtpSenders');
							t.equal(rtpSender.associatedPeer, alice, '`receiverPeer` must be alice');

							let transport = bob.transports[0];

							// (allow "make functions within a loop")
							// jshint -W083
							rtpSender.setTransport(transport)
								.then(() =>
								{
									t.pass('rtpSender.setTransport() succeeded');
									t.equal(rtpSender.transport, transport, 'rtpSender.transport must retrieve the given `transport`');

									if (++handledRtpSenders === EXPECTED_RTPSENDERS)
										accept();
								})
								.catch((error) =>
								{
									t.fail(`rtpSender.setTransport() failed: ${error}`);
									reject(error);
								});
							// jshint +W083
						}
					});

					let promise2 = new Promise((accept, reject) =>
					{
						if (availableRtpSenders.length === EXPECTED_RTPSENDERS)
						{
							accept();
							return;
						}

						bob.on('newrtpsender', (rtpSender) =>
						{
							t.pass('bob "newrtpsender" event fired');
							t.equal(rtpSender.associatedPeer, alice, '`rtpSender.associatedPeer` must be alice');

							let transport = bob.transports[0];

							rtpSender.setTransport(transport)
								.then(() =>
								{
									t.pass('rtpSender.setTransport() succeeded');
									t.equal(rtpSender.transport, transport, 'rtpSender.transport must retrieve the given `transport`');

									if (++handledRtpSenders === EXPECTED_RTPSENDERS)
										accept();
								})
								.catch((error) =>
								{
									t.fail(`rtpSender.setTransport() failed: ${error}`);
									reject(error);
								});
						});
					});

					return Promise.all([ promise1, promise2 ]);
				})
		])
		.then(() =>
		{
			// Remove listeners
			alice.removeAllListeners('newrtpsender');
			bob.removeAllListeners('newrtpsender');

			t.equal(alice.rtpSenders.length, 1, 'alice.rtpSenders must retrieve 1');
			t.equal(bob.rtpSenders.length, 2, 'bob.rtpSenders must retrieve 2');

			t.same(alice.rtpSenders[0].rtpParameters, bobVideoParameters,
				'first RtpSender parameters of alice must match video parameters of bob');
			t.same(bob.rtpSenders[0].rtpParameters, aliceAudioParameters,
				'first RtpSender parameters of bob must match audio parameters of alice');
			t.same(bob.rtpSenders[1].rtpParameters, aliceVideoParameters,
				'second RtpSender parameters of bob must match video parameters of alice');
		})
		.then(() =>
		{
			let aliceAudioReceiver = alice.rtpReceivers[0];
			let aliceVideoReceiver = alice.rtpReceivers[1];
			let aliceVideoSender = alice.rtpSenders[0];
			let bobVideoReceiver = bob.rtpReceivers[0];
			let bobAudioSender = bob.rtpSenders[0];
			let bobVideoSender = bob.rtpSenders[1];

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

				let aliceAudioReceiver = alice.rtpReceivers[0];
				let bobAudioSender = bob.rtpSenders[0];

				aliceAudioReceiver.on('close', () => t2.pass('aliceAudioReceiver closed'));
				bobAudioSender.on('close', () => t2.pass('bobAudioSender closed'));

				aliceAudioReceiver.close();
			});
		})
		.then(() =>
		{
			let aliceVideoReceiver = alice.rtpReceivers[0];
			let aliceVideoSender = alice.rtpSenders[0];
			let bobVideoReceiver = bob.rtpReceivers[0];
			let bobVideoSender = bob.rtpSenders[0];

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

				let aliceVideoSender = alice.rtpSenders[0];
				let bobVideoReceiver = bob.rtpReceivers[0];
				let bobVideoSender = bob.rtpSenders[0];

				bob.on('close', () => t2.pass('bob closed'));
				bobVideoReceiver.on('close', () => t2.pass('bobVideoReceiver closed'));
				bobVideoSender.on('close', () => t2.pass('bobVideoSender closed'));
				aliceVideoSender.on('close', () => t2.pass('aliceVideoSender closed'));

				bob.close();
			});
		})
		.then(() =>
		{
			t.equal(alice.rtpReceivers.length, 1, 'alice.rtpReceivers must retrieve 1');
			t.equal(bob.rtpReceivers.length, 0, 'bob.rtpReceivers must retrieve 0');
			t.equal(alice.rtpSenders.length, 0, 'alice.rtpSenders must retrieve 0');
			t.equal(bob.rtpSenders.length, 0, 'bob.rtpSenders must retrieve 0');

			let aliceVideoReceiver = alice.rtpReceivers[0];

			return room.dump()
				.then((data) =>
				{
					t.pass('room.dump() succeeded');

					t.same(data.mapRtpReceiverRtpSenders,
						{
							[aliceVideoReceiver._internal.rtpReceiverId] : []
						},
						'`data.mapRtpReceiverRtpSenders` match the expected values');
				})
				.catch((error) => t.fail(`room.dump() failed: ${error}`));
		})
		.then(() =>
		{
			carol = room.Peer('carol');

			return createTransportAndRtpReceivers(t, carol, [ carolAudioParameters, carolVideoParameters ]);
		})
		.then(() =>
		{
			let aliceVideoReceiver = alice.rtpReceivers[0];
			let aliceAudioSender = alice.rtpSenders[0];
			let aliceVideoSender = alice.rtpSenders[1];
			let carolAudioReceiver = carol.rtpReceivers[0];
			let carolVideoReceiver = carol.rtpReceivers[1];
			let carolVideoSender = carol.rtpSenders[0];

			return room.dump()
				.then((data) =>
				{
					t.pass('room.dump() succeeded');

					t.same(data.mapRtpReceiverRtpSenders,
						{
							[aliceVideoReceiver._internal.rtpReceiverId] :
								[
									String(carolVideoSender._internal.rtpSenderId)
								],
							[carolAudioReceiver._internal.rtpReceiverId] :
								[
									String(aliceAudioSender._internal.rtpSenderId)
								],
							[carolVideoReceiver._internal.rtpReceiverId] :
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
			t.end();
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
