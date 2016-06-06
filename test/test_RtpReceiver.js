'use strict';

const tap = require('tap');

const mediasoup = require('../');
const promiseSeries = require('./utils/promiseSeries');

function initTest(t)
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	return peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			return { peer: peer, transport: transport };
		});
}

tap.test('rtpReceiver.receive() with no encodings must succeed', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);

			t.equal(rtpReceiver.transport, transport, 'rtpReceiver.transport must retrieve the given transport');

			return rtpReceiver.receive(
				{
					codecs :
					[
						{
							name        : 'audio/opus',
							payloadType : 100
						},
						{
							name        : 'audio/rtx',
							payloadType : 98,
							parameters  :
							{
								apt : 100
							}
						}
					]
				})
				.then(() =>
				{
					t.pass('rtpReceiver.receive() succeeded');

					let rtpParameters = rtpReceiver.rtpParameters;

					t.assert(rtpParameters.encodings, 'computed rtpParameters has encodings');
					t.equal(rtpParameters.encodings.length, 1, 'encodings has 1 encoding');
					t.equal(rtpParameters.encodings[0].codecPayloadType, 100, 'encoding has codecPayloadType 100');
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		});
});

tap.test('rtpReceiver.receive() with encodings without codecPayloadType must succeed', { timeout: 2000 }, (t) =>
{
	return initTest(t)
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
							name        : 'video/H264',
							payloadType : 101
						},
						{
							name        : 'video/H265',
							payloadType : 102
						}
					],
					encodings :
					[
						{
							ssrc : 1111
						},
						{
							codecPayloadType : 102
						},
						{
							ssrc : 3333
						}
					]
				})
				.then(() =>
				{
					t.pass('rtpReceiver.receive() succeeded');

					let rtpParameters = rtpReceiver.rtpParameters;

					t.equal(rtpParameters.encodings.length, 3, 'computed rtpParameters has 3 encodings');
					t.equal(rtpParameters.encodings[0].codecPayloadType, 101, 'first encoding has codecPayloadType 101');
					t.equal(rtpParameters.encodings[1].codecPayloadType, 102, 'second encoding has codecPayloadType 102');
					t.equal(rtpParameters.encodings[2].codecPayloadType, 101, 'third encoding has codecPayloadType 101');
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		});
});

tap.test('rtpReceiver.receive() with full rtpParameters must succeed', { timeout: 2000 }, (t) =>
{
	return initTest(t)
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
						name         : 'video/VP9',
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
							aaa               : 1.0,
							foo               : 'barœæ€',
							bar               : true,
							baz               : -123,
							lol               : -456.789
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
						resolutionScale       : 2,
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
						uri        : 'urn:ietf:params:rtp-hdrext:foo',
						id         : 1234,
						encrypt    : false,
						parameters : {}
					},
					{
						uri        : 'urn:ietf:params:rtp-hdrext:bar',
						id         : 5678,
						encrypt    : true,
						parameters : {}
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
					foo : -123,
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

					t.same(rtpReceiver.rtpParameters, rtpParameters, 'computed rtpParameters match given ones');
				})
				.catch((error) => t.fail(`rtpReceiver.receive() failed: ${error}`));
		});
});

tap.test('two rtpReceiver.receive() over the same transport sharing PT values must succeed if ssrc are given', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver1 = peer.RtpReceiver('audio', transport);
			let rtpReceiver2 = peer.RtpReceiver('audio', transport);
			let promises = [];

			promises.push(rtpReceiver1.receive(
				{
					codecs :
					[
						{
							name        : 'audio/opus',
							payloadType : 101,
							clockRate   : 90000
						},
						{
							name        : 'audio/rtx',
							payloadType : 102,
							parameters  :
							{
								apt : 101
							}
						}
					],
					encodings :
					[
						{
							ssrc : 1001
						}
					]
				})
				.then(() =>
				{
					t.pass('rtpReceiver1.receive() succeeded');
				}));

			promises.push(rtpReceiver2.receive(
				{
					codecs :
					[
						{
							name        : 'audio/opus',
							payloadType : 101,
							clockRate   : 90000
						}
					],
					encodings :
					[
						{
							ssrc : 1002
						}
					]
				})
				.then(() =>
				{
					t.pass('rtpReceiver2.receive() succeeded');
				}));

			return Promise.all(promises);
		});
});

tap.test('rtpReceiver.receive() without rtpParameters must fail', { timeout: 2000 }, (t) =>
{
	return initTest(t)
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

tap.test('rtpReceiver.receive() with wrong codecs must fail', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);
			let funcs = [];

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'opus',
								payloadType : 101
							}
						]
					})
					.then(() => t.fail('rtpReceiver.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`rtpReceiver.receive() with an invalid codec.name failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : '/opus',
								payloadType : 101
							}
						]
					})
					.then(() => t.fail('rtpReceiver.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`rtpReceiver.receive() with an invalid codec.name failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/',
								payloadType : 101
							}
						]
					})
					.then(() => t.fail('rtpReceiver.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`rtpReceiver.receive() with an invalid codec.name failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
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
						t.pass(`rtpReceiver.receive() without codec.name failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
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
						t.pass(`rtpReceiver.receive() without codec.payloadType failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 101
							},
							{
								name        : 'audio/opus',
								payloadType : 101
							}
						]
					})
					.then(() => t.fail('rtpReceiver.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`rtpReceiver.receive() with duplicated codec.payloadType failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100
							},
							{
								name        : 'audio/rtx',
								payloadType : 101
							}
						]
					})
					.then(() => t.fail('rtpReceiver.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`rtpReceiver.receive() with incomplete RTX codec failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100
							},
							{
								name        : 'audio/rtx',
								payloadType : 101,
								parameters  :
								{
									apt : 98
								}
							}
						]
					})
					.then(() => t.fail('rtpReceiver.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`rtpReceiver.receive() with wrong RTX codec failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100
							},
							{
								name        : 'audio/ulpfec',
								payloadType : 101
							},
							{
								name        : 'audio/rtx',
								payloadType : 97,
								parameters  :
								{
									apt : 101
								}
							}
						]
					})
					.then(() => t.fail('rtpReceiver.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`rtpReceiver.receive() with RTX codec pointing to a ULPFEC codec failed: ${error}`);
					});
			});

			return promiseSeries(funcs);
		});
});

tap.test('rtpReceiver.receive() with wrong encodings must fail', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);
			let funcs = [];

			funcs.push(function()
			{
				return rtpReceiver.receive(
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
						t.pass(`rtpReceiver.receive() with unknown encoding.codecPayloadType failed: ${error}`);
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 101
							},
							{
								name        : 'audio/CN',
								payloadType : 102
							}
						],
						encodings :
						[
							{
								codecPayloadType : 102
							}
						]
					})
					.then(() => t.fail('rtpReceiver.receive() with invalid encoding.codecPayloadType succeeded'))
					.catch((error) =>
					{
						t.pass(`rtpReceiver.receive() failed: ${error}`);
					});
			});

			return promiseSeries(funcs);
		});
});

tap.test('two rtpReceiver.receive() over the same transport sharing PT values must fail if ssrc are not given', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver1 = peer.RtpReceiver('audio', transport);
			let rtpReceiver2 = peer.RtpReceiver('audio', transport);
			let promises = [];

			promises.push(rtpReceiver1.receive(
				{
					codecs :
					[
						{
							name        : 'audio/opus',
							payloadType : 101,
							clockRate   : 90000
						}
					]
				})
				.then(() =>
				{
					t.pass('rtpReceiver1.receive() succeeded');
				}));

			promises.push(rtpReceiver2.receive(
				{
					codecs :
					[
						{
							name        : 'audio/PCMU',
							payloadType : 101,
							clockRate   : 90000
						}
					]
				})
				.then(() => t.fail('rtpReceiver2.receive() succeeded'))
				.catch((error) =>
				{
					t.pass(`rtpReceiver2.receive() failed: ${error}`);
				}));

			return Promise.all(promises);
		});
});

tap.test('rtpReceiver.receive() should produce the expected RTP listener routing tables', { timeout: 2000 }, (t) =>
{
	return initTest(t)
		.then((data) =>
		{
			let peer = data.peer;
			let transport = data.transport;
			let rtpReceiver = peer.RtpReceiver('audio', transport);
			let funcs = [];

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100
							}
						]
					})
					.then(() =>
					{
						return transport.dump();
					})
					.then((data) =>
					{
						let id = rtpReceiver._internal.rtpReceiverId;

						t.same(data.rtpListener,
							{
								muxIdTable : {},
								ptTable    :
								{
									100 : id
								},
								ssrcTable  : {}
							},
							'rtpListener tables match expected ones');
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						muxId  : 'qwerty1234',
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100
							},
							{
								name        : 'audio/PCMU',
								payloadType : 0
							}
						]
					})
					.then(() =>
					{
						return transport.dump();
					})
					.then((data) =>
					{
						let id = rtpReceiver._internal.rtpReceiverId;

						t.same(data.rtpListener,
							{
								muxIdTable :
								{
									'qwerty1234' : id
								},
								ptTable    :
								{
									100 : id,
									0   : id
								},
								ssrcTable  : {}
							},
							'rtpListener tables match expected ones');
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100
							},
							{
								name        : 'audio/PCMU',
								payloadType : 0
							}
						],
						encodings :
						[
							{
								codecPayloadType : 100,
								ssrc             : 1111
							},
							{
								codecPayloadType : 0
							}
						]
					})
					.then(() =>
					{
						return transport.dump();
					})
					.then((data) =>
					{
						let id = rtpReceiver._internal.rtpReceiverId;

						t.same(data.rtpListener,
							{
								muxIdTable : {},
								ptTable    :
								{
									100 : id,
									0   : id
								},
								ssrcTable  :
								{
									1111 : id
								}
							},
							'rtpListener tables match expected ones');
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100
							},
							{
								name        : 'audio/PCMU',
								payloadType : 0
							}
						],
						encodings :
						[
							{
								codecPayloadType : 100,
								ssrc             : 1111
							},
							{
								codecPayloadType : 0,
								ssrc             : 2222
							}
						]
					})
					.then(() =>
					{
						return transport.dump();
					})
					.then((data) =>
					{
						let id = rtpReceiver._internal.rtpReceiverId;

						t.same(data.rtpListener,
							{
								muxIdTable : {},
								ptTable    : {},
								ssrcTable  :
								{
									1111 : id,
									2222 : id
								}
							},
							'rtpListener tables match expected ones');
					});
			});

			funcs.push(function()
			{
				return rtpReceiver.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100
							},
							{
								name        : 'audio/PCMU',
								payloadType : 0
							}
						],
						encodings :
						[
							{
								codecPayloadType : 100,
								ssrc             : 1111,
								rtx              : {}
							},
							{
								codecPayloadType : 0,
								ssrc             : 2222,
								fec              :
								{
									mechanism : 'flexfec',
									ssrc      : 3333
								}
							}
						]
					})
					.then(() =>
					{
						return transport.dump();
					})
					.then((data) =>
					{
						let id = rtpReceiver._internal.rtpReceiverId;

						t.same(data.rtpListener,
							{
								muxIdTable : {},
								ptTable    :
								{
									100 : id,
									0   : id
								},
								ssrcTable  :
								{
									1111 : id,
									2222 : id,
									3333 : id
								}
							},
							'rtpListener tables match expected ones');
					});
			});

			return promiseSeries(funcs);
		});
});

tap.test('rtpReceiver.close() must succeed', { timeout: 2000 }, (t) =>
{
	return initTest(t)
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
