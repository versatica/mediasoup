'use strict';

const tap = require('tap');

const mediasoup = require('../');

tap.test('transport.createAssociatedTransport() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`))
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');
			t.equal(transport.iceComponent, 'RTP', 'transport must have "RTP" `iceComponent`');

			transport.iceLocalCandidates.forEach((candidate) =>
			{
				if (candidate.protocol !== 'udp')
					t.fail('transport contains unexpected "tcp" candidates');
			});
			t.pass('transport just contains "udp" candidates');

			transport.createAssociatedTransport()
				.catch((error) => t.fail(`transport.createAssociatedTransport() failed: ${error}`))
				.then((associatedTransport) =>
				{
					t.pass('transport.createAssociatedTransport() succeeded');
					t.equal(associatedTransport.iceComponent, 'RTCP', 'associated transport must have "RTCP" `iceComponent`');

					associatedTransport.iceLocalCandidates.forEach((candidate) =>
					{
						if (candidate.protocol !== 'udp')
							t.fail('associated transport contains unexpected "tcp" candidates');
					});
					t.pass('associated transport just contains "udp" candidates');

					peer.dump()
						.catch((error) => t.fail(`peer.dump() failed: ${error}`))
						.then((data) =>
						{
							t.pass('peer.dump() succeeded');
							t.equal(Object.keys(data.transports).length, 2, 'peer.dump() should retrieve two transports');
							t.end();
						});
				});
		});
});

tap.test('transport.close() must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.catch((error) => t.fail(`peer.createTransport() failed: ${error}`))
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.on('close', (error) =>
			{
				t.error(error, 'transport should close cleanly');

				peer.dump()
					.catch((error) => t.fail(`peer.dump() failed: ${error}`))
					.then((data) =>
					{
						t.pass('peer.dump() succeeded');
						t.equal(Object.keys(data.transports).length, 0, 'peer.dump() should retrieve zero transports');
						t.end();
					});
			});

			setTimeout(() => transport.close(), 100);
		});
});

tap.test('transport.setRemoteDtlsParameters() with "server" `role` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`))
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');
			t.equal(transport.dtlsLocalParameters.role, 'server', 'default local DTLS `role` must be "server"');

			transport.setRemoteDtlsParameters(
				{
					role        : 'server',
					fingerprint :
					{
						algorithm : 'sha-1',
						value     : '751b8193b7ed277e42bed6c48ef7043a49ce3faa'
					}
				})
				.catch((error) => t.fail(`transport.createAssociatedTransport() failed: ${error}`))
				.then(() =>
				{
					t.pass('transport.setRemoteDtlsParameters() succeeded');
					t.equal(transport.dtlsLocalParameters.role, 'client', 'new local DTLS `role` must be "client"');

					transport.dump()
						.catch((error) => t.fail(`peer.dump() failed: ${error}`))
						.then((data) =>
						{
							t.pass('transport.dump() succeeded');
							t.equal(data.dtlsLocalParameters.role, 'client', 'local DTLS `role` must be "client"');
							t.end();
						});
				});
		});
});

tap.test('transport.setRemoteDtlsParameters() with "server" `auto` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`))
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
				.catch((error) => t.fail(`transport.createAssociatedTransport() failed: ${error}`))
				.then(() =>
				{
					t.pass('transport.setRemoteDtlsParameters() succeeded');
					t.equal(transport.dtlsLocalParameters.role, 'client', 'new local DTLS `role` must be "client"');

					transport.dump()
						.catch((error) => t.fail(`peer.dump() failed: ${error}`))
						.then((data) =>
						{
							t.pass('transport.dump() succeeded');
							t.equal(data.dtlsLocalParameters.role, 'client', 'local DTLS `role` must be "client"');
							t.end();
						});
				});
		});
});

tap.test('transport.setRemoteDtlsParameters() with no `role` must succeed', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`))
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
				.catch((error) => t.fail(`transport.createAssociatedTransport() failed: ${error}`))
				.then(() =>
				{
					t.pass('transport.setRemoteDtlsParameters() succeeded');
					t.equal(transport.dtlsLocalParameters.role, 'client', 'new local DTLS `role` must be "client"');

					transport.dump()
						.catch((error) => t.fail(`peer.dump() failed: ${error}`))
						.then((data) =>
						{
							t.pass('transport.dump() succeeded');
							t.equal(data.dtlsLocalParameters.role, 'client', 'local DTLS `role` must be "client"');
							t.end();
						});
				});
		});
});

tap.test('transport.setRemoteDtlsParameters() with invalid `role` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`))
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
				.catch((error) =>
				{
					t.pass(`transport.createAssociatedTransport() failed: ${error}`);
					t.equal(transport.dtlsLocalParameters.role, 'server', 'local DTLS `role` must be "server"');
					t.end();
				})
				.then(() => t.fail('transport.setRemoteDtlsParameters() succeeded'));
		});
});

tap.test('transport.setRemoteDtlsParameters() without `fingerprint` must fail', { timeout: 1000 }, (t) =>
{
	let server = mediasoup.Server();

	t.tearDown(() => server.close());

	let room = server.Room();
	let peer = room.Peer('alice');

	peer.createTransport({ tcp: false })
		.catch((error) => t.fail(`peer.createTransport failed: ${error}`))
		.then((transport) =>
		{
			t.pass('peer.createTransport() succeeded');

			transport.setRemoteDtlsParameters(
				{
					role        : 'server'
				})
				.catch((error) =>
				{
					t.pass(`transport.createAssociatedTransport() failed: ${error}`);
					t.equal(transport.dtlsLocalParameters.role, 'server', 'local DTLS `role` must be "server"');
					t.end();
				})
				.then(() => t.fail('transport.setRemoteDtlsParameters() succeeded'));
		});
});
