'use strict';

const tap = require('tap');
const mediasoup = require('../');
// const roomOptions = require('./data/options').roomOptions;

tap.test(
	'server.Room() with valid media codecs', { timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server();

		t.tearDown(() => server.close());

		const mediaCodecs =
		[
			{
				kind      : 'audio',
				name      : 'opus',
				clockRate : 48000,
				channels  : 2
			},
			{
				kind      : 'audio',
				name      : 'PCMU',
				clockRate : 8000
			},
			{
				kind       : 'video',
				name       : 'VP8',
				clockRate  : 90000,
				parameters :
				{
					foo : 123
				}
			}
		];

		const expectedCodecs =
		[
			{
				kind                 : 'audio',
				name                 : 'opus',
				mimeType             : 'audio/opus',
				preferredPayloadType : 100,
				clockRate            : 48000,
				channels             : 2,
				parameters           : {}
			},
			{
				kind                 : 'audio',
				name                 : 'PCMU',
				mimeType             : 'audio/PCMU',
				preferredPayloadType : 0,
				clockRate            : 8000,
				channels             : 1,
				parameters           : {}
			},
			{
				kind                 : 'video',
				name                 : 'VP8',
				mimeType             : 'video/VP8',
				preferredPayloadType : 101,
				clockRate            : 90000,
				parameters           :
				{
					foo : 123
				}
			},
			{
				kind                 : 'video',
				name                 : 'rtx',
				mimeType             : 'video/rtx',
				preferredPayloadType : 102,
				clockRate            : 90000,
				parameters           :
				{
					apt : 101
				}
			}
		];

		const room = server.Room(mediaCodecs);
		const reducedCodecs = room.rtpCapabilities.codecs
			.map((codec) =>
			{
				delete codec.rtcpFeedback;

				return codec;
			});

		t.same(reducedCodecs, expectedCodecs, 'room codecs match');
		t.end();
	});

// tap.test(
// 	'server.updateSettings() with no options must succeed', { timeout: 2000 }, (t) =>
// 	{
// 		const server = mediasoup.Server();

// 		t.tearDown(() => server.close());

// 		server.updateSettings()
// 			.then(() =>
// 			{
// 				t.pass('server.updateSettings() succeeded');
// 				t.end();
// 			})
// 			.catch((error) => t.fail(`server.updateSettings() failed: ${error}`));
// 	});

// tap.test(
// 	'server.updateSettings() with valid options must succeed',
// 	{ timeout: 2000 }, (t) =>
// 	{
// 		const server = mediasoup.Server();

// 		t.tearDown(() => server.close());

// 		server.updateSettings({ logLevel: 'warn' })
// 			.then(() =>
// 			{
// 				t.pass('server.updateSettings() succeeded');
// 				t.end();
// 			})
// 			.catch((error) => t.fail(`server.updateSettings() failed: ${error}`));
// 	});

// tap.test(
// 	'server.updateSettings() with invalid options must fail', { timeout: 2000 }, (t) =>
// 	{
// 		const server = mediasoup.Server();

// 		t.tearDown(() => server.close());

// 		server.updateSettings({ logLevel: 'chicken' })
// 			.then(() => t.fail('server.updateSettings() succeeded'))
// 			.catch((error) =>
// 			{
// 				t.pass(`server.updateSettings() failed: ${error}`);
// 				t.end();
// 			});
// 	});

// tap.test(
// 	'server.updateSettings() in a closed server must fail', { timeout: 2000 }, (t) =>
// 	{
// 		const server = mediasoup.Server();

// 		t.tearDown(() => server.close());

// 		server.on('close', () =>
// 		{
// 			server.updateSettings({ logLevel: 'error' })
// 				.then(() => t.fail('server.updateSettings() succeeded'))
// 				.catch((error) =>
// 				{
// 					t.pass(`server.updateSettings() failed: ${error}`);
// 					t.type(
// 						error, mediasoup.errors.InvalidStateError,
// 						'server.updateSettings() must reject with InvalidStateError');
// 					t.end();
// 				});
// 		});

// 		server.close();
// 	});

// tap.test(
// 	'server.createRoom() must succeed', { _timeout: 2000 }, (t) =>
// 	{
// 		const server = mediasoup.Server();

// 		t.tearDown(() => server.close());

// 		server.createRoom(roomOptions)
// 			.then((room) =>
// 			{
// 				t.pass('server.createRoom() succeeded');

// 				room.on('close', (error) =>
// 				{
// 					t.error(error, 'room must close cleanly');
// 					t.end();
// 				});

// 				// Wait a bit so Channel requests receive response
// 				setTimeout(() => room.close(), 50);
// 			})
// 			.catch((error) => t.fail(`server.createRoom() failed: ${error}`));
// 	});

// tap.test(
// 	'server.createRoom() in a closed server must fail', { timeout: 2000 }, (t) =>
// 	{
// 		const server = mediasoup.Server();

// 		server.close();

// 		server.createRoom(roomOptions)
// 			.then(() => t.fail('server.createRoom() succeeded'))
// 			.catch((error) =>
// 			{
// 				t.pass(`server.createRoom() failed: ${error}`);
// 				t.type(
// 					error, mediasoup.errors.InvalidStateError,
// 					'server.createRoom() must reject with InvalidStateError');
// 				t.end();
// 			});
// 	});

// tap.test(
// 	'server.dump() must succeed', { timeout: 2000 }, (t) =>
// 	{
// 		const server = mediasoup.Server({ numWorkers: 2 });

// 		t.tearDown(() => server.close());

// 		server.dump()
// 			.then((data) =>
// 			{
// 				t.pass('server.dump() succeeded');
// 				t.equal(
// 					Object.keys(data.workers).length, 2,
// 					'server.dump() must retrieve two workers');
// 				t.end();
// 			})
// 			.catch((error) => t.fail(`server.dump() failed: ${error}`));
// 	});
