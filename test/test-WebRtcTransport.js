const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;

expect.extend({ toBeType });

let worker;
let router;
let transport;

const mediaCodecs =
[
	{
		kind       : 'audio',
		name       : 'opus',
		mimeType   : 'audio/opus',
		clockRate  : 48000,
		channels   : 2,
		parameters :
		{
			useinbandfec : 1,
			foo          : 'bar'
		}
	},
	{
		kind      : 'video',
		name      : 'VP8',
		clockRate : 90000
	},
	{
		kind         : 'video',
		name         : 'H264',
		mimeType     : 'video/H264',
		clockRate    : 90000,
		rtcpFeedback : [], // Will be ignored.
		parameters   :
		{
			'level-asymmetry-allowed' : 1,
			'packetization-mode'      : 1,
			'profile-level-id'        : '4d0032',
			foo                       : 'bar'
		}
	}
];

beforeAll(async () =>
{
	worker = await createWorker();
	router = await worker.createRouter({ mediaCodecs });
});

afterAll(() => worker.close());

beforeEach(async () =>
{
	transport = await router.createWebRtcTransport(
		{
			listenIps : [ { ip: '127.0.0.1', announcedIp: '9.9.9.1' } ],
			enableTcp : false
		});
});

afterEach(() => transport.close());

test('router.createWebRtcTransport() succeeds', async () =>
{
	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				id                       : router.id,
				transportIds             : [ transport.id ],
				mapProducerIdConsumerIds : {},
				mapConsumerIdProducerId  : {}
			});

	// Create a separate transport here.
	const transport1 = await router.createWebRtcTransport(
		{
			listenIps :
			[
				{ ip: '127.0.0.1', announcedIp: '9.9.9.1' },
				{ ip: '0.0.0.0', announcedIp: '9.9.9.2' },
				{ ip: '127.0.0.1' }
			],
			enableTcp : true,
			preferUdp : true,
			appData   : { foo: 'bar' }
		});

	expect(transport1.id).toBeType('string');
	expect(transport1.closed).toBe(false);
	expect(transport1.appData).toEqual({ foo: 'bar' });
	expect(transport1.iceRole).toBe('controlled');
	expect(transport1.iceLocalParameters).toBeType('object');
	expect(transport1.iceLocalParameters.iceLite).toBe(true);
	expect(transport1.iceLocalParameters.usernameFragment).toBeType('string');
	expect(transport1.iceLocalParameters.password).toBeType('string');
	expect(transport1.iceLocalCandidates).toBeType('array');
	expect(transport1.iceLocalCandidates.length).toBe(6);

	const iceCandidates = transport1.iceLocalCandidates;

	expect(iceCandidates[0].ip).toBe('9.9.9.1');
	expect(iceCandidates[0].protocol).toBe('udp');
	expect(iceCandidates[0].type).toBe('host');
	expect(iceCandidates[0].tcpType).toBe(undefined);
	expect(iceCandidates[1].ip).toBe('9.9.9.1');
	expect(iceCandidates[1].protocol).toBe('tcp');
	expect(iceCandidates[1].type).toBe('host');
	expect(iceCandidates[1].tcpType).toBe('passive');
	expect(iceCandidates[2].ip).toBe('9.9.9.2');
	expect(iceCandidates[2].protocol).toBe('udp');
	expect(iceCandidates[2].type).toBe('host');
	expect(iceCandidates[2].tcpType).toBe(undefined);
	expect(iceCandidates[3].ip).toBe('9.9.9.2');
	expect(iceCandidates[3].protocol).toBe('tcp');
	expect(iceCandidates[3].type).toBe('host');
	expect(iceCandidates[3].tcpType).toBe('passive');
	expect(iceCandidates[4].ip).toBe('127.0.0.1');
	expect(iceCandidates[4].protocol).toBe('udp');
	expect(iceCandidates[4].type).toBe('host');
	expect(iceCandidates[4].tcpType).toBe(undefined);
	expect(iceCandidates[5].ip).toBe('127.0.0.1');
	expect(iceCandidates[5].protocol).toBe('tcp');
	expect(iceCandidates[5].type).toBe('host');
	expect(iceCandidates[5].tcpType).toBe('passive');
	expect(iceCandidates[0].priority).toBeGreaterThan(iceCandidates[1].priority);
	expect(iceCandidates[2].priority).toBeGreaterThan(iceCandidates[1].priority);
	expect(iceCandidates[2].priority).toBeGreaterThan(iceCandidates[3].priority);
	expect(iceCandidates[4].priority).toBeGreaterThan(iceCandidates[3].priority);
	expect(iceCandidates[4].priority).toBeGreaterThan(iceCandidates[5].priority);
	expect(transport1.iceState).toBe('new');
	expect(transport1.iceSelectedTuple).toBe(undefined);
	expect(transport1.dtlsLocalParameters).toBeType('object');
	expect(transport1.dtlsLocalParameters.fingerprints).toBeType('array');
	expect(transport1.dtlsLocalParameters.role).toBe('auto');
	expect(transport1.dtlsState).toBe('new');
	expect(transport1.dtlsRemoteCert).toBe(undefined);

	const data1 = await transport1.dump();

	expect(data1.id).toBe(transport1.id);
	expect(data1.producerIds).toEqual([]);
	expect(data1.consumerIds).toEqual([]);
	expect(data1.iceRole).toBe(transport1.iceRole);
	expect(data1.iceLocalParameters).toEqual(transport1.iceLocalParameters);
	expect(data1.iceLocalCandidates).toEqual(transport1.iceLocalCandidates);
	expect(data1.iceState).toBe(transport1.iceState);
	expect(data1.iceSelectedTuple).toEqual(transport1.iceSelectedTuple);
	expect(data1.dtlsLocalParameters).toEqual(transport1.dtlsLocalParameters);
	expect(data1.dtlsState).toBe(transport1.dtlsState);
	expect(data1.rtpHeaderExtensions).toBeType('object');
	expect(data1.rtpListener).toBeType('object');

	expect(router.getTransportById(transport1.id)).toBe(transport1);
	transport1.close();
	expect(transport1.closed).toBe(true);
	expect(router.getTransportById(transport1.id)).toBe(undefined);

	await expect(router.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] }))
		.resolves
		.toBeType('object');
}, 2000);

test('router.createWebRtcTransport() with wrong options rejects with TypeError', async () =>
{
	await expect(router.createWebRtcTransport())
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport({ listenIps: [] }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport({ listenIps: [ 123 ] }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport({ listenIps: '127.0.0.1' }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ],
			appData   : 'NOT-AN-OBJECT'
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('router.createWebRtcTransport() with non bindable IP rejects with Error', async () =>
{
	await expect(router.createWebRtcTransport({ listenIps: [ '8.8.8.8' ] }))
		.rejects
		.toThrow(Error);
}, 2000);

test('webRtcTransport.getStats() succeeds', async () =>
{
	const data = await transport.getStats();

	expect(data).toBeType('array');
	expect(data.length).toBe(1);
	expect(data[0].type).toBe('transport');
	expect(data[0].transportId).toBeType('string');
	expect(data[0].timestamp).toBeType('number');
	expect(data[0].iceRole).toBe('controlled');
	expect(data[0].iceState).toBe('new');
	expect(data[0].dtlsState).toBe('new');
	expect(data[0].bytesReceived).toBe(0);
	expect(data[0].bytesSent).toBe(0);
	expect(data[0].iceSelectedTuple).toBe(undefined);
	expect(data[0].availableIncomingBitrate).toBe(0);
	expect(data[0].availableOutgoingBitrate).toBe(0);
	expect(data[0].maxIncomingBitrate).toBe(undefined);
}, 2000);

test('webRtcTransport.connect() succeeds', async () =>
{
	const dtlsRemoteParameters =
	{
		fingerprints :
		[
			{
				algorithm : 'sha-256',
				value     : '82:5A:68:3D:36:C3:0A:DE:AF:E7:32:43:D2:88:83:57:AC:2D:65:E5:80:C4:B6:FB:AF:1A:A0:21:9F:6D:0C:AD'
			}
		],
		role : 'client'
	};

	await expect(transport.connect({ dtlsParameters: dtlsRemoteParameters }))
		.resolves
		.toBe(undefined);

	// Must fail if connected.
	await expect(transport.connect({ dtlsParameters: dtlsRemoteParameters }))
		.rejects
		.toThrow(Error);

	expect(transport.dtlsLocalParameters.role).toBe('server');
}, 2000);

test('webRtcTransport.connect() with wrong arguments rejects with TypeError', async () =>
{
	let dtlsRemoteParameters;

	await expect(transport.connect())
		.rejects
		.toThrow(TypeError);

	dtlsRemoteParameters =
	{
		fingerprints :
		[
			{
				algorithm : 'sha-256000',
				value     : '82:5A:68:3D:36:C3:0A:DE:AF:E7:32:43:D2:88:83:57:AC:2D:65:E5:80:C4:B6:FB:AF:1A:A0:21:9F:6D:0C:AD'
			}
		],
		role : 'client'
	};

	await expect(transport.connect({ dtlsParameters: dtlsRemoteParameters }))
		.rejects
		.toThrow(TypeError);

	dtlsRemoteParameters =
	{
		fingerprints :
		[
			{
				algorithm : 'sha-256',
				value     : '82:5A:68:3D:36:C3:0A:DE:AF:E7:32:43:D2:88:83:57:AC:2D:65:E5:80:C4:B6:FB:AF:1A:A0:21:9F:6D:0C:AD'
			}
		],
		role : 'chicken'
	};

	await expect(transport.connect({ dtlsParameters: dtlsRemoteParameters }))
		.rejects
		.toThrow(TypeError);

	await expect(transport.connect({ dtlsParameters: dtlsRemoteParameters }))
		.rejects
		.toThrow(TypeError);

	expect(transport.dtlsLocalParameters.role).toBe('auto');
}, 2000);

test('webRtcTransport.setMaxIncomingBitrate() succeeds', async () =>
{
	await expect(transport.setMaxIncomingBitrate(100000))
		.resolves
		.toBe(undefined);
}, 2000);

test('webRtcTransport.restartIce() succeeds', async () =>
{
	const previousIceUsernameFragment = transport.iceLocalParameters.usernameFragment;
	const previousIcePassword = transport.iceLocalParameters.password;

	await expect(transport.restartIce())
		.resolves
		.toBeType('object');

	expect(transport.iceLocalParameters.usernameFragment).toBeType('string');
	expect(transport.iceLocalParameters.password).toBeType('string');
	expect(transport.iceLocalParameters.usernameFragment)
		.not.toBe(previousIceUsernameFragment);
	expect(transport.iceLocalParameters.password)
		.not.toBe(previousIcePassword);
}, 2000);

test('WebRtcTransport events succeed', async () =>
{
	// Private API.
	const channel = transport._channel;

	const onIceStateChange = jest.fn();

	transport.on('icestatechange', onIceStateChange);

	channel.emit(transport.id, 'icestatechange', { iceState: 'completed' });

	expect(onIceStateChange).toHaveBeenCalledTimes(1);
	expect(onIceStateChange).toHaveBeenCalledWith('completed');
	expect(transport.iceState).toBe('completed');

	const onIceSelectedTuple = jest.fn();
	const iceSelectedTuple =
	{
		localIp    : '1.1.1.1',
		localPort  : 1111,
		remoteIp   : '2.2.2.2',
		remotePort : 2222,
		protocol   : 'udp'
	};

	transport.on('iceselectedtuplechange', onIceSelectedTuple);

	channel.emit(transport.id, 'iceselectedtuplechange', { iceSelectedTuple });

	expect(onIceSelectedTuple).toHaveBeenCalledTimes(1);
	expect(onIceSelectedTuple).toHaveBeenCalledWith(iceSelectedTuple);
	expect(transport.iceSelectedTuple).toEqual(iceSelectedTuple);

	const onDtlsStateChange = jest.fn();

	transport.on('dtlsstatechange', onDtlsStateChange);

	channel.emit(transport.id, 'dtlsstatechange', { dtlsState: 'connecting' });

	expect(onDtlsStateChange).toHaveBeenCalledTimes(1);
	expect(onDtlsStateChange).toHaveBeenCalledWith('connecting');
	expect(transport.dtlsState).toBe('connecting');

	channel.emit(
		transport.id, 'dtlsstatechange', { dtlsState: 'connected', dtlsRemoteCert: 'ABCD' });

	expect(onDtlsStateChange).toHaveBeenCalledTimes(2);
	expect(onDtlsStateChange).toHaveBeenCalledWith('connected');
	expect(transport.dtlsState).toBe('connected');
	expect(transport.dtlsRemoteCert).toBe('ABCD');
}, 2000);

test('WebRtcTransport methods reject if closed', async () =>
{
	transport.close();

	expect(transport.closed).toBe(true);

	await expect(transport.dump())
		.rejects
		.toThrow(Error);

	await expect(transport.getStats())
		.rejects
		.toThrow(Error);

	await expect(transport.connect())
		.rejects
		.toThrow(Error);

	await expect(transport.setMaxIncomingBitrate())
		.rejects
		.toThrow(Error);

	await expect(transport.restartIce())
		.rejects
		.toThrow(Error);
}, 2000);

test('WebRtcTransport emits "routerclose" if Router is closed', async () =>
{
	// We need different Router and WebRtcTransport instances here.
	const router2 = await worker.createRouter({ mediaCodecs });
	const transport2 =
		await router2.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] });

	await new Promise((resolve) =>
	{
		transport2.on('routerclose', resolve);

		router2.close();
	});

	expect(transport2.closed).toBe(true);
}, 2000);

test('WebRtcTransport emits "routerclose" if Worker is closed', async () =>
{
	await new Promise((resolve) =>
	{
		transport.on('routerclose', resolve);

		worker.close();
	});

	expect(transport.closed).toBe(true);
}, 2000);
