'use strict';

const tap = require('tap');

const mediasoup = require('../');
const roomOptions = require('./data/options').roomOptions;
const peerRtpCapabilities = require('./data/options').peerRtpCapabilities;

tap.test('transport.setRemoteDtlsParameters() with "server" role must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);
	let peer = room.Peer('alice', peerRtpCapabilities);

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');
			t.equal(transport.dtlsLocalParameters.role, 'auto', 'default local DTLS role must be "auto"');

			transport.setRemoteDtlsParameters(
				{
					role        : 'server',
					fingerprint :
					{
						algorithm : 'sha-1',
						value     : '751b8193b7ed277e42bed6c48ef7043a49ce3faa'
					}
				})
				.then(() =>
				{
					t.pass('transport.setRemoteDtlsParameters() succeeded');
					t.equal(transport.dtlsLocalParameters.role, 'client', 'new local DTLS role must be "client"');

					transport.dump()
						.then((data) =>
						{
							t.pass('transport.dump() succeeded');
							t.equal(data.dtlsLocalParameters.role, 'client', 'local DTLS role must be "client"');
							t.end();
						})
						.catch((error) => t.fail(`peer.dump() failed: ${error}`));
				})
				.catch((error) => t.fail(`transport.setRemoteDtlsParameters() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

tap.test('transport.setRemoteDtlsParameters() with "auto" role must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);
	let peer = room.Peer('alice', peerRtpCapabilities);

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.setRemoteDtlsParameters(
				{
					role        : 'auto',
					fingerprint :
					{
						algorithm : 'sha-1',
						value     : '751b8193b7ed277e42bed6c48ef7043a49ce3faa'
					}
				})
				.then(() =>
				{
					t.pass('transport.setRemoteDtlsParameters() succeeded');
					t.equal(transport.dtlsLocalParameters.role, 'client', 'new local DTLS role must be "client"');

					transport.dump()
						.then((data) =>
						{
							t.pass('transport.dump() succeeded');
							t.equal(data.dtlsLocalParameters.role, 'client', 'local DTLS role must be "client"');
							t.end();
						})
						.catch((error) => t.fail(`peer.dump() failed: ${error}`));
				})
				.catch((error) => t.fail(`transport.setRemoteDtlsParameters() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

tap.test('transport.setRemoteDtlsParameters() with no role must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);
	let peer = room.Peer('alice', peerRtpCapabilities);

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.setRemoteDtlsParameters(
				{
					fingerprint :
					{
						algorithm : 'sha-1',
						value     : '751b8193b7ed277e42bed6c48ef7043a49ce3faa'
					}
				})
				.then(() =>
				{
					t.pass('transport.setRemoteDtlsParameters() succeeded');
					t.equal(transport.dtlsLocalParameters.role, 'client', 'new local DTLS role must be "client"');

					transport.dump()
						.then((data) =>
						{
							t.pass('transport.dump() succeeded');
							t.equal(data.dtlsLocalParameters.role, 'client', 'local DTLS role must be "client"');
							t.end();
						})
						.catch((error) => t.fail(`peer.dump() failed: ${error}`));
				})
				.catch((error) => t.fail(`transport.setRemoteDtlsParameters() failed: ${error}`));
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

tap.test('transport.setRemoteDtlsParameters() with invalid role must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);
	let peer = room.Peer('alice', peerRtpCapabilities);

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.setRemoteDtlsParameters(
				{
					role        : 'INVALID_ROLE',
					fingerprint :
					{
						algorithm : 'sha-1',
						value     : '751b8193b7ed277e42bed6c48ef7043a49ce3faa'
					}
				})
				.then(() => t.fail('transport.setRemoteDtlsParameters() succeeded'))
				.catch((error) =>
				{
					t.pass(`transport.setRemoteDtlsParameters() failed: ${error}`);
					t.equal(transport.dtlsLocalParameters.role, 'auto', 'local DTLS role must be "auto"');
					t.end();
				});
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

tap.test('transport.setRemoteDtlsParameters() without fingerprint must fail', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);
	let peer = room.Peer('alice', peerRtpCapabilities);

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.setRemoteDtlsParameters(
				{
					role : 'server'
				})
				.then(() => t.fail('transport.setRemoteDtlsParameters() succeeded'))
				.catch((error) =>
				{
					t.pass(`transport.setRemoteDtlsParameters() failed: ${error}`);
					t.equal(transport.dtlsLocalParameters.role, 'auto', 'local DTLS role must be "auto"');
					t.end();
				});
		})
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`));
});

tap.test('transport.close() must succeed', { timeout: 2000 }, (t) =>
{
	let server = mediasoup.Server();

	t.plan(6);
	t.tearDown(() => server.close());

	let room = server.Room(roomOptions);
	let peer = room.Peer('alice', peerRtpCapabilities);

	peer.createTransport({ tcp: false })
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.on('close', (error) =>
			{
				t.error(error, 'transport must close cleanly');
				t.equal(transport.iceState, 'closed', 'transport.iceState must be "closed"');
				t.equal(transport.dtlsState, 'closed', 'transport.dtlsState must be "closed"');

				peer.dump()
					.then((data) =>
					{
						t.pass('peer.dump() succeeded');
						t.equal(Object.keys(data.transports).length, 0, 'peer.dump() must retrieve zero transports');
					})
					.catch((error) => t.fail(`peer.dump() failed: ${error}`));
			});

			setTimeout(() => transport.close(), 100);
		})
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`));
});
