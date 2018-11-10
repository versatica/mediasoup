const tap = require('tap');
const mediasoup = require('../');

tap.test(
	'server.Room() with valid media codecs must succeed', { timeout: 2000 }, (t) =>
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
				name       : 'vp8',
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

tap.test(
	'server.Room() with empty media codecs must fail', { timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server();

		t.tearDown(() => server.close());

		t.throws(
			() =>
			{
				// eslint-disable-next-line no-unused-vars
				const room = server.Room([]);
			},
			TypeError,
			'server.Room() must throw TypeError');

		t.end();
	});

tap.test(
	'server.updateSettings() with no options must succeed', { timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server();

		t.tearDown(() => server.close());

		server.updateSettings()
			.then(() =>
			{
				t.pass('server.updateSettings() succeeded');
				t.end();
			})
			.catch((error) => t.fail(`server.updateSettings() failed: ${error}`));
	});

tap.test(
	'server.updateSettings() with valid options must succeed',
	{ timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server();

		t.tearDown(() => server.close());

		server.updateSettings({ logLevel: 'warn' })
			.then(() =>
			{
				t.pass('server.updateSettings() succeeded');
				t.end();
			})
			.catch((error) => t.fail(`server.updateSettings() failed: ${error}`));
	});

tap.test(
	'server.updateSettings() with invalid options must fail', { timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server();

		t.tearDown(() => server.close());

		server.updateSettings({ logLevel: 'chicken' })
			.then(() => t.fail('server.updateSettings() succeeded'))
			.catch((error) =>
			{
				t.pass(`server.updateSettings() failed: ${error}`);
				t.end();
			});
	});

tap.test(
	'server.updateSettings() in a closed server must fail', { timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server();

		t.tearDown(() => server.close());

		server.on('close', () =>
		{
			server.updateSettings({ logLevel: 'error' })
				.then(() => t.fail('server.updateSettings() succeeded'))
				.catch((error) =>
				{
					t.pass(`server.updateSettings() failed: ${error}`);
					t.type(
						error, mediasoup.errors.InvalidStateError,
						'server.updateSettings() must reject with InvalidStateError');
					t.end();
				});
		});

		server.close();
	});
