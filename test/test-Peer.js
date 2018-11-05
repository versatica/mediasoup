const tap = require('tap');
const mediasoup = require('../');
const roomOptions = require('./data/options').roomOptions;
const peerCapabilities = require('./data/options').peerCapabilities;

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
			return { peer };
		});
}

tap.test(
	'peer.createTransport() with no options must succeed', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;

				t.equal(peer.name, 'alice', 'peer.name must be "alice"');

				return peer.createTransport()
					.then(() =>
					{
						t.pass('peer.createTransport() succeeded');

						return peer.dump()
							.then((data2) =>
							{
								t.pass('peer.dump() succeeded');
								t.equal(
									Object.keys(data2.transports).length, 1,
									'peer.dump() must retrieve one transport');
								t.end();
							})
							.catch((error) => t.fail(`peer.dump() failed: ${error}`));
					});
			});
	});

tap.test(
	'peer.createTransport() with no udp nor tcp must fail', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;

				return peer.createTransport({ udp: false, tcp: false })
					.then(() => t.fail('peer.createTransport() succeeded'))
					.catch((error) =>
					{
						t.pass(`peer.createTransport() failed: ${error}`);
					});
			});
	});

tap.test(
	'peer.Producer() with valid transport must succeed', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;
				let producer;

				return peer.createTransport({ tcp: false })
					.then((transport) =>
					{
						t.pass('peer.createTransport() succeeded');

						producer = peer.Producer('audio', transport);
						t.pass('peer.Producer() succeeded');

						t.equal(
							producer.transport, transport,
							'producer.transport must retrieve the given transport');

						return peer.dump();
					})
					.then((data2) =>
					{
						t.pass('peer.dump() succeeded');
						t.equal(
							Object.keys(data2.producers).length, 1,
							'peer.dump() must retrieve one producer');
						t.same(
							peer.producers[0], producer,
							'peer.producers[0] must retrieve the previous producer');
					});
			});
	});

tap.test(
	'peer.Producer() with a closed transport must fail', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;

				return peer.createTransport({ tcp: false })
					.then((transport) =>
					{
						t.pass('peer.createTransport() succeeded');

						transport.close();

						t.throws(() =>
						{
							peer.Producer('audio', transport);
						},
						mediasoup.errors.InvalidStateError,
						'peer.Producer() must throw InvalidStateError');
					});
			});
	});

tap.test(
	'peer.Producer() with invalid kind must fail', { timeout: 2000 }, (t) =>
	{
		return initTest(t)
			.then((data) =>
			{
				const peer = data.peer;

				return peer.createTransport({ tcp: false })
					.then((transport) =>
					{
						t.pass('peer.createTransport() succeeded');

						t.throws(() =>
						{
							peer.Producer('chicken', transport);
						},
						TypeError,
						'peer.Producer() must throw TypeError');
					});
			});
	});
