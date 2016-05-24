'use strict';

const tap = require('tap');

const mediasoup = require('../');

function init(test)
{
	let server = mediasoup.Server();

	test.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	return peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			return { peer: peer, transport: transport };
		});
}

tap.test('rtpReceiver.receive() with no `rtpParameters.encodings` must succeed', { timeout: 1000 }, (t) =>
{
	return init(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);

			t.equal(rtpReceiver.transport, transport, 'rtpReceiver.transport must retrieve the given `transport`');

			return rtpReceiver.receive(
				{
					codecs :
					[
						{
							name         : 'audio/opus',
							payloadType  : 100
						}
					]
				})
				.then(() =>
				{
					t.pass('rtpReceiver.receive() succeeded');

					let rtpParameters = rtpReceiver.rtpParameters;

					t.assert(rtpParameters.encodings, 'computed `rtpParameters` has `encodings`');
					t.equal(rtpParameters.encodings.length, 1, '`encodings` has 1 `encoding`');
					t.equal(rtpParameters.encodings[0].codecPayloadType, 100, '`encoding` has matching `codecPayloadType`');
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		});
});

tap.test('rtpReceiver.receive() with one `rtpParameters.encodings` without `codecPayloadType` must succeed', { timeout: 1000 }, (t) =>
{
	return init(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('video', transport);

			return rtpReceiver.receive(
				{
					codecs :
					[
						{
							name         : 'video/H264',
							payloadType  : 101
						}
					],
					encodings :
					[
						{
							ssrc : 1234
						}
					]
				})
				.then(() =>
				{
					t.pass('rtpReceiver.receive() succeeded');

					let rtpParameters = rtpReceiver.rtpParameters;

					t.equal(rtpParameters.encodings.length, 1, 'computed `rtpParameters` has 1 `encoding`');
					t.equal(rtpParameters.encodings[0].codecPayloadType, 101, '`encoding` has matching `codecPayloadType`');
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		});
});

tap.test('rtpReceiver.receive() with full `rtpParameters` must succeed', { timeout: 1000 }, (t) =>
{
	return init(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
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
						resolutionScale       : 2.2,
						framerateScale        : 1.5,
						maxFramerate          : 30,
						active                : false,
						encodingId            : 'ENC3',
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

			return rtpReceiver.receive(rtpParameters)
				.then(() =>
				{
					t.pass('rtpReceiver.receive() succeeded');

					t.same(rtpReceiver.rtpParameters, rtpParameters, 'computed `rtpParameters` match given ones');
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		});
});

tap.test('rtpReceiver.receive() without `rtpParameters` must fail', { timeout: 1000 }, (t) =>
{
	return init(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);

			return rtpReceiver.receive()
				.then(() => t.fail('rtpReceiver.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver.receive() failed: ${error}`);
				});
		});
});

tap.test('rtpReceiver.receive() with wrong `codecs` must fail', { timeout: 1000 }, (t) =>
{
	return init(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);
			let promises = [];

			promises.push(rtpReceiver.receive(
				{
					codecs :
					[
						{
							name        : 'chicken/FOO',
							payloadType : 101
						}
					]
				})
				.then(() => t.fail('rtpReceiver.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver.receive() failed: ${error}`);
				}));

			promises.push(rtpReceiver.receive(
				{
					codecs :
					[
						{
							payloadType : 102
						}
					]
				})
				.then(() => t.fail('rtpReceiver.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver.receive() failed: ${error}`);
				}));

			promises.push(rtpReceiver.receive(
				{
					codecs :
					[
						{
							name : 'audio/opus'
						}
					]
				})
				.then(() => t.fail('rtpReceiver.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver.receive() failed: ${error}`);
				}));

			return Promise.all(promises);
		});
});

tap.test('rtpReceiver.receive() with wrong `encodings` must fail', { timeout: 1000 }, (t) =>
{
	return init(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);
			let promises = [];

			promises.push(rtpReceiver.receive(
				{
					codecs :
					[
						{
							name        : 'audio/opus',
							payloadType : 101
						}
					],
					encodings :
					[
						{
							codecPayloadType : 102
						}
					]
				})
				.then(() => t.fail('rtpReceiver.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver.receive() failed: ${error}`);
				}));

			promises.push(rtpReceiver.receive(
				{
					codecs :
					[
						{
							name        : 'audio/rtx',
							payloadType : 101
						}
					],
					encodings :
					[
						{
							codecPayloadType : 101
						}
					]
				})
				.then(() => t.fail('rtpReceiver.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver.receive() failed: ${error}`);
				}));

			promises.push(rtpReceiver.receive(
				{
					codecs :
					[
						{
							name        : 'audio/rtx',
							payloadType : 101
						},
						{
							name        : 'audio/opus',
							payloadType : 102
						}
					],
					encodings :
					[
						{
							codecPayloadType : 101
						}
					]
				})
				.then(() => t.fail('rtpReceiver.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver.receive() failed: ${error}`);
				}));

			return Promise.all(promises);
		});
});

tap.test('rtpReceiver.close() must succeed', { timeout: 1000 }, (t) =>
{
	return init(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);

			setTimeout(() => rtpReceiver.close(), 100);

			return new Promise((accept, reject) =>
			{
				rtpReceiver.on('close', (error) =>
				{
					t.error(error, 'rtpReceiver must close cleanly');
					t.equal(peer.rtpReceivers.length, 0, 'peer must have 0 rtpReceivers');

					accept();
				});
			});
		});
});
