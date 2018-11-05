const tap = require('tap');
const mediasoup = require('../');
const roomOptions = require('./data/options').roomOptions;
const peerCapabilities = require('./data/options').peerCapabilities;
const promiseSeries = require('./utils/promiseSeries');

function initTest(t)
{
	const server = mediasoup.Server();
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
			return peer.createTransport({ tcp: false });
		})
		.then((transport) =>
		{
			return { peer, transport };
		});
}

tap.test(
	'producer.receive() with no encodings must succeed', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer = peer.Producer('audio', transport);

				t.equal(producer.peer, peer, 'associated peer must match');

				return producer.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100,
								clockRate   : 48000
							}
						]
					})
					.then(() =>
					{
						t.pass('producer.receive() succeeded');

						const rtpParameters = producer.rtpParameters;

						t.assert(
							rtpParameters.encodings,
							'computed rtpParameters has encodings');
						t.equal(
							rtpParameters.encodings.length, 1,
							'encodings has 1 encoding');
						t.equal(
							rtpParameters.encodings[0].codecPayloadType, 100,
							'encoding has codecPayloadType 100');
					})
					.catch((error) => t.fail(`producer.receive() failed: ${error}`));
			});
	});

tap.test(
	'producer.receive() with encodings without codecPayloadType must succeed',
	{ timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer = peer.Producer('video', transport);

				return producer.receive(
					{
						codecs :
						[
							{
								kind        : 'video',
								name        : 'video/vp8',
								payloadType : 110,
								clockRate   : 90000
							},
							{
								name        : 'video/H264',
								payloadType : 112,
								clockRate   : 90000,
								parameters  :
								{
									packetizationMode : 1
								}
							}
						],
						encodings :
						[
							{
								ssrc : 1111
							},
							{
								codecPayloadType : 112
							},
							{
								ssrc : 3333
							}
						]
					})
					.then(() =>
					{
						t.pass('producer.receive() succeeded');

						const rtpParameters = producer.rtpParameters;

						t.equal(
							rtpParameters.encodings.length, 3,
							'computed rtpParameters has 3 encodings');
						t.equal(
							rtpParameters.encodings[0].codecPayloadType,
							110, 'first encoding has codecPayloadType 110');
						t.equal(
							rtpParameters.encodings[1].codecPayloadType,
							112, 'second encoding has codecPayloadType 112');
						t.equal(
							rtpParameters.encodings[2].codecPayloadType,
							110, 'third encoding has codecPayloadType 110');
					})
					.catch((error) => t.fail(`producer.receive() failed: ${error}`));
			});
	});

tap.test(
	'producer.receive() with full rtpParameters must succeed',
	{ timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer = peer.Producer('video', transport);
				const rtpParameters =
				{
					muxId  : 'abcd',
					codecs :
					[
						{
							name         : 'video/VP8',
							payloadType  : 110,
							clockRate    : 90000,
							maxptime     : 80,
							ptime        : 60,
							numChannels  : 2,
							rtcpFeedback :
							[
								{ type: 'nack', parameter: null },
								{ type: 'nack', parameter: 'pli' },
								{ type: 'nack', parameter: 'sli' },
								{ type: 'nack', parameter: 'rpsi' },
								{ type: 'nack', parameter: 'app' },
								{ type: 'ccm', parameter: 'fir' },
								{ type: 'ack', parameter: 'rpsi' },
								{ type: 'ack', parameter: 'app' }
							],
							parameters :
							{
								profileLevelId    : 2,
								packetizationMode : 1,
								aaa               : 1.0,
								foo               : 'barœæ€',
								bar               : true,
								baz               : -123,
								lol               : -456.789,
								ids               : [ 123, 2, 3 ]
							}
						}
					],
					encodings :
					[
						{
							ssrc             : 100000,
							codecPayloadType : 110,
							fec              :
							{
								ssrc      : 200000,
								mechanism : 'foo'
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
							uri        : 'urn:ietf:params:rtp-hdrext:toffset',
							id         : 2,
							encrypt    : false,
							parameters : {}
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

				return producer.receive(rtpParameters)
					.then(() =>
					{
						t.pass('producer.receive() succeeded');

						t.same(
							producer.rtpParameters, rtpParameters,
							'computed rtpParameters match given ones');
					})
					.catch((error) => t.fail(`producer.receive() failed: ${error}`));
			});
	});

tap.test(
	'two producer.receive() over the same transport sharing PT values must ' +
	'succeed if ssrc are given',
	{ timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer1 = peer.Producer('audio', transport);
				const producer2 = peer.Producer('audio', transport);
				const promises = [];

				promises.push(producer1.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100,
								clockRate   : 48000
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
						t.pass('producer1.receive() succeeded');
					}));

				promises.push(producer2.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100,
								clockRate   : 48000
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
						t.pass('producer2.receive() succeeded');
					}));

				return Promise.all(promises);
			});
	});

tap.test(
	'producer.receive() without rtpParameters must fail', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer = peer.Producer('audio', transport);

				return producer.receive()
					.then(() => t.fail('producer.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`producer.receive() failed: ${error}`);
					});
			});
	});

tap.test(
	'producer.receive() with wrong codecs must fail', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer = peer.Producer('audio', transport);
				const funcs = [];

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'opus',
									payloadType : 100
								}
							]
						})
						.then(() => t.fail('producer.receive() succeeded'))
						.catch((error) =>
						{
							t.pass(
								`producer.receive() with an invalid codec.name failed: ${error}`);
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : '/opus',
									payloadType : 100,
									clockRate   : 48000
								}
							]
						})
						.then(() => t.fail('producer.receive() succeeded'))
						.catch((error) =>
						{
							t.pass(
								`producer.receive() with an invalid codec.name failed: ${error}`);
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'audio/',
									payloadType : 100,
									clockRate   : 48000
								}
							]
						})
						.then(() => t.fail('producer.receive() succeeded'))
						.catch((error) =>
						{
							t.pass(
								`producer.receive() with an invalid codec.name failed: ${error}`);
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									payloadType : 102,
									clockRate   : 90000
								}
							]
						})
						.then(() => t.fail('producer.receive() succeeded'))
						.catch((error) =>
						{
							t.pass(`producer.receive() without codec.name failed: ${error}`);
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name      : 'audio/opus',
									clockRate : 48000
								}
							]
						})
						.then(() => t.fail('producer.receive() succeeded'))
						.catch((error) =>
						{
							t.pass(
								`producer.receive() without codec.payloadType failed: ${error}`);
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'audio/opus',
									payloadType : 100
								}
							]
						})
						.then(() => t.fail('producer.receive() succeeded'))
						.catch((error) =>
						{
							t.pass(
								`producer.receive() without codec.clockRate failed: ${error}`);
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'audio/opus',
									payloadType : 100,
									clockRate   : 48000
								},
								{
									name        : 'audio/opus',
									payloadType : 100,
									clockRate   : 48000
								}
							]
						})
						.then(() => t.fail('producer.receive() succeeded'))
						.catch((error) =>
						{
							t.pass(
								'producer.receive() with duplicated codec.payloadType failed: ' +
								`${error}`);
						});
				});

				// TODO: Must fix this.
				// funcs.push(function()
				// {
				// 	return producer.receive(
				// 		{
				// 			codecs :
				// 			[
				// 				{
				// 					kind        : 'video',
				// 					name        : 'video/vp8',
				// 					payloadType : 110,
				// 					clockRate   : 90000
				// 				},
				// 				{
				// 					kind        : 'video',
				// 					name        : 'video/rtx',
				// 					payloadType : 97,
				// 					clockRate   : 90000,
				// 					parameters  :
				// 					{
				// 						apt : 111 // Wrong apt!
				// 					}
				// 				}
				// 			]
				// 		})
				// 		.then(() => t.fail('producer.receive() succeeded'))
				// 		.catch((error) =>
				// 		{
				// 			t.pass(`producer.receive() wih wrong RTX apt failed: ${error}`);
				// 		});
				// });

				return promiseSeries(funcs);
			});
	});

tap.test(
	'producer.receive() with wrong encodings must fail', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer = peer.Producer('audio', transport);
				const funcs = [];

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'audio/opus',
									payloadType : 100,
									clockRate   : 48000
								}
							],
							encodings :
							[
								{
									codecPayloadType : 102
								}
							]
						})
						.then(() => t.fail('producer.receive() succeeded'))
						.catch((error) =>
						{
							t.pass(
								'producer.receive() with unknown encoding.codecPayloadType ' +
								`failed: ${error}`);
						});
				});

				return promiseSeries(funcs);
			});
	});

tap.test(
	'two producer.receive() over the same transport sharing PT values must fail ' +
	'if ssrc are not given',
	{ timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer1 = peer.Producer('audio', transport);
				const producer2 = peer.Producer('audio', transport);
				const promises = [];

				promises.push(producer1.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100,
								clockRate   : 48000
							}
						]
					})
					.then(() =>
					{
						t.pass('producer1.receive() succeeded');
					}));

				promises.push(producer2.receive(
					{
						codecs :
						[
							{
								name        : 'audio/opus',
								payloadType : 100,
								clockRate   : 48000
							}
						]
					})
					.then(() => t.fail('producer2.receive() succeeded'))
					.catch((error) =>
					{
						t.pass(`producer2.receive() failed: ${error}`);
					}));

				return Promise.all(promises);
			});
	});

tap.test(
	'producer.receive() should produce the expected RTP listener routing tables',
	{ timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer = peer.Producer('audio', transport);
				const funcs = [];

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'audio/opus',
									payloadType : 100,
									clockRate   : 48000
								}
							]
						})
						.then(() =>
						{
							return transport.dump();
						})
						.then((data2) =>
						{
							const id = producer._internal.producerId;

							t.same(data2.rtpListener,
								{
									muxIdTable : {},
									ptTable    :
									{
										100 : id
									},
									ssrcTable : {}
								},
								'rtpListener tables match expected ones');
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							muxId  : 'qwerty1234',
							codecs :
							[
								{
									name        : 'audio/opus',
									payloadType : 100,
									clockRate   : 48000
								},
								{
									name        : 'audio/PCMU',
									payloadType : 0,
									clockRate   : 8000
								}
							]
						})
						.then(() =>
						{
							return transport.dump();
						})
						.then((data2) =>
						{
							const id = producer._internal.producerId;

							t.same(data2.rtpListener,
								{
									muxIdTable :
									{
										'qwerty1234' : id
									},
									ptTable :
									{
										100 : id,
										0   : id
									},
									ssrcTable : {}
								},
								'rtpListener tables match expected ones');
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'audio/opus',
									payloadType : 100,
									clockRate   : 48000
								},
								{
									name        : 'audio/PCMU',
									payloadType : 0,
									clockRate   : 8000
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
						.then((data2) =>
						{
							const id = producer._internal.producerId;

							t.same(data2.rtpListener,
								{
									muxIdTable : {},
									ptTable    :
									{
										100 : id,
										0   : id
									},
									ssrcTable :
									{
										1111 : id
									}
								},
								'rtpListener tables match expected ones');
						});
				});

				funcs.push(function()
				{
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'audio/opus',
									payloadType : 100,
									clockRate   : 48000
								},
								{
									name        : 'audio/PCMU',
									payloadType : 0,
									clockRate   : 8000
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
						.then((data2) =>
						{
							const id = producer._internal.producerId;

							t.same(data2.rtpListener,
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
					return producer.receive(
						{
							codecs :
							[
								{
									name        : 'audio/opus',
									payloadType : 100,
									clockRate   : 48000
								},
								{
									name        : 'audio/PCMU',
									payloadType : 0,
									clockRate   : 8000
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
						.then((data2) =>
						{
							const id = producer._internal.producerId;

							t.same(data2.rtpListener,
								{
									muxIdTable : {},
									ptTable    :
									{
										100 : id,
										0   : id
									},
									ssrcTable :
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

tap.test(
	'producer.close() must succeed', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				const transport = data.transport;
				const producer = peer.Producer('audio', transport);

				setTimeout(() => producer.close(), 100);

				return new Promise((accept, reject) => // eslint-disable-line no-unused-vars
				{
					producer.on('close', (error) =>
					{
						t.error(error, 'producer must close cleanly');
						t.equal(peer.producers.length, 0, 'peer must have 0 producers');

						accept();
					});
				});
			});
	});
