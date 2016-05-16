'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('rtpReceiver.receive() with valid `rtpParameters` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			let rtpReceiver = peer.RtpReceiver('video', transport);
			let rtpParameters =
			{
				muxId  : 'abcd',
				codecs :
				[
					{
						name         : 'video/H264',
						payloadType  : 100,
						clockRate    : 90000,
						maxptime     : 80,
						ptime        : 60,
						numChannels  : 2,
						rtcpFeedback :
						[
							{ type: 'ccm',         parameter: 'fir' },
							{ type: 'nack',        parameter: '' },
							{ type: 'nack',        parameter: 'pli' },
							{ type: 'google-remb', parameter: '' }
						],
						parameters :
						{
							profileLevelId    : 2,
							packetizationMode : 1,
							foo               : 'barœæ€',
							isGood            : true
						}
					}
				],
				encodings :
				[
					{
						ssrc             : 100000,
						codecPayloadType : 100,
						fec :
						{
							ssrc      : 200000,
							mechanism : 'foo'
						},
						rtx :
						{
							ssrc : 300000
						},
						resolutionScale  : 2.2,
						framerateScale   : 1.5,
						maxFramerate     : 30,
						active           : false,
						encodingId       : 'ENC3',
						dependencyEncodingIds : [ 'ENC1', 'ENC2' ]
					}
				],
				headerExtensions :
				[
					{
						uri     : 'urn:ietf:params:rtp-hdrext:foo',
						id      : 1234,
						encrypt : false
					},
					{
						uri     : 'urn:ietf:params:rtp-hdrext:bar',
						id      : 5678,
						encrypt : true
					},
					{
						uri        : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
						id         : 6,
						encrypt    : false,
						parameters :
						{
							vad : 'on'
						}
					}
				],
				rtcp :
				{
					cname       : 'a7sdihkj3sdsdflqwkejl98ujk',
					ssrc        : 88888888,
					reducedSize : true
				},
				userParameters :
				{
					foo : 123,
					bar : 'BAR',
					baz : true,
					cas : -123.123,
					arr : [ 1, '2', 3.3, { qwe: 'QWE', asd: 123, zxc: null } ]
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
								.then(() =>
								{
									t.pass('transport.dump() succeeded');
									// TODO: Enable when done
									// t.same(Object.keys(data.rtpListener.ssrcTable).sort(), [ ... ].sort(), 'transport.dump() must provide the given ssrc values');
									// t.same(Object.keys(data.rtpListener.ptTable).sort(), [ ... ].sort(), 'transport.dump() must provide the given payload types');
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
			let rtpReceiver = peer.RtpReceiver('audio', transport);

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
			let rtpReceiver = peer.RtpReceiver('video', transport);

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
