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
		mimeType  : 'video/VP8',
		clockRate : 90000
	},
	{
		kind       : 'video',
		mimeType   : 'video/H264',
		clockRate  : 90000,
		parameters :
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
		.toMatchObject({ transportIds: [ transport.id ] });

	const onObserverNewTransport = jest.fn();

	router.observer.once('newtransport', onObserverNewTransport);

	// Create a separate transport here.
	const transport1 = await router.createWebRtcTransport(
		{
			listenIps :
			[
				{ ip: '127.0.0.1', announcedIp: '9.9.9.1' },
				{ ip: '0.0.0.0', announcedIp: '9.9.9.2' },
				{ ip: '127.0.0.1', announcedIp: null }
			],
			enableTcp          : true,
			preferUdp          : true,
			enableSctp         : true,
			numSctpStreams     : { OS: 2048, MIS: 2048 },
			maxSctpMessageSize : 1000000,
			appData            : { foo: 'bar' }
		});

	expect(onObserverNewTransport).toHaveBeenCalledTimes(1);
	expect(onObserverNewTransport).toHaveBeenCalledWith(transport1);
	expect(transport1.id).toBeType('string');
	expect(transport1.closed).toBe(false);
	expect(transport1.appData).toEqual({ foo: 'bar' });
	expect(transport1.iceRole).toBe('controlled');
	expect(transport1.iceParameters).toBeType('object');
	expect(transport1.iceParameters.iceLite).toBe(true);
	expect(transport1.iceParameters.usernameFragment).toBeType('string');
	expect(transport1.iceParameters.password).toBeType('string');
	expect(transport1.sctpParameters).toStrictEqual(
		{
			port               : 5000,
			OS                 : 2048,
			MIS                : 2048,
			maxMessageSize     : 1000000,
			isDataChannel      : true,
			sctpBufferedAmount : 0,
			sendBufferSize     : 262144
		});
	expect(transport1.iceCandidates).toBeType('array');
	expect(transport1.iceCandidates.length).toBe(6);

	const iceCandidates = transport1.iceCandidates;

	expect(iceCandidates[0].ip).toBe('9.9.9.1');
	expect(iceCandidates[0].protocol).toBe('udp');
	expect(iceCandidates[0].type).toBe('host');
	expect(iceCandidates[0].tcpType).toBeUndefined();
	expect(iceCandidates[1].ip).toBe('9.9.9.1');
	expect(iceCandidates[1].protocol).toBe('tcp');
	expect(iceCandidates[1].type).toBe('host');
	expect(iceCandidates[1].tcpType).toBe('passive');
	expect(iceCandidates[2].ip).toBe('9.9.9.2');
	expect(iceCandidates[2].protocol).toBe('udp');
	expect(iceCandidates[2].type).toBe('host');
	expect(iceCandidates[2].tcpType).toBeUndefined();
	expect(iceCandidates[3].ip).toBe('9.9.9.2');
	expect(iceCandidates[3].protocol).toBe('tcp');
	expect(iceCandidates[3].type).toBe('host');
	expect(iceCandidates[3].tcpType).toBe('passive');
	expect(iceCandidates[4].ip).toBe('127.0.0.1');
	expect(iceCandidates[4].protocol).toBe('udp');
	expect(iceCandidates[4].type).toBe('host');
	expect(iceCandidates[4].tcpType).toBeUndefined();
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
	expect(transport1.iceSelectedTuple).toBeUndefined();
	expect(transport1.dtlsParameters).toBeType('object');
	expect(transport1.dtlsParameters.fingerprints).toBeType('array');
	expect(transport1.dtlsParameters.role).toBe('auto');
	expect(transport1.dtlsState).toBe('new');
	expect(transport1.dtlsRemoteCert).toBeUndefined();
	expect(transport1.sctpState).toBe('new');

	const data1 = await transport1.dump();

	expect(data1.id).toBe(transport1.id);
	expect(data1.direct).toBe(false);
	expect(data1.producerIds).toEqual([]);
	expect(data1.consumerIds).toEqual([]);
	expect(data1.iceRole).toBe(transport1.iceRole);
	expect(data1.iceParameters).toEqual(transport1.iceParameters);
	expect(data1.iceCandidates).toEqual(transport1.iceCandidates);
	expect(data1.iceState).toBe(transport1.iceState);
	expect(data1.iceSelectedTuple).toEqual(transport1.iceSelectedTuple);
	expect(data1.dtlsParameters).toEqual(transport1.dtlsParameters);
	expect(data1.dtlsState).toBe(transport1.dtlsState);
	expect(data1.sctpParameters).toEqual(transport1.sctpParameters);
	expect(data1.sctpState).toBe(transport1.sctpState);
	expect(data1.recvRtpHeaderExtensions).toBeType('object');
	expect(data1.rtpListener).toBeType('object');

	transport1.close();
	expect(transport1.closed).toBe(true);

	await expect(router.createWebRtcTransport({ listenIps: [ '127.0.0.1' ] }))
		.resolves
		.toBeType('object');
}, 2000);

test('router.createWebRtcTransport() with wrong arguments rejects with TypeError', async () =>
{
	await expect(router.createWebRtcTransport({}))
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

	await expect(router.createWebRtcTransport(
		{
			listenIps      : [ '127.0.0.1' ],
			enableSctp     : true,
			numSctpStreams : 'foo'
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
	expect(data[0].type).toBe('webrtc-transport');
	expect(data[0].transportId).toBeType('string');
	expect(data[0].timestamp).toBeType('number');
	expect(data[0].iceRole).toBe('controlled');
	expect(data[0].iceState).toBe('new');
	expect(data[0].dtlsState).toBe('new');
	expect(data[0].sctpState).toBeUndefined();
	expect(data[0].bytesReceived).toBe(0);
	expect(data[0].recvBitrate).toBe(0);
	expect(data[0].bytesSent).toBe(0);
	expect(data[0].sendBitrate).toBe(0);
	expect(data[0].rtpBytesReceived).toBe(0);
	expect(data[0].rtpRecvBitrate).toBe(0);
	expect(data[0].rtpBytesSent).toBe(0);
	expect(data[0].rtpSendBitrate).toBe(0);
	expect(data[0].rtxBytesReceived).toBe(0);
	expect(data[0].rtxRecvBitrate).toBe(0);
	expect(data[0].rtxBytesSent).toBe(0);
	expect(data[0].rtxSendBitrate).toBe(0);
	expect(data[0].probationBytesSent).toBe(0);
	expect(data[0].probationSendBitrate).toBe(0);
	expect(data[0].iceSelectedTuple).toBeUndefined();
	expect(data[0].maxIncomingBitrate).toBeUndefined();
	expect(data[0].recvBitrate).toBe(0);
	expect(data[0].sendBitrate).toBe(0);
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
		.toBeUndefined();

	// Must fail if connected.
	await expect(transport.connect({ dtlsParameters: dtlsRemoteParameters }))
		.rejects
		.toThrow(Error);

	expect(transport.dtlsParameters.role).toBe('server');
}, 2000);

test('webRtcTransport.connect() with wrong arguments rejects with TypeError', async () =>
{
	let dtlsRemoteParameters;

	await expect(transport.connect({}))
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

	expect(transport.dtlsParameters.role).toBe('auto');
}, 2000);

test('webRtcTransport.setMaxIncomingBitrate() succeeds', async () =>
{
	await expect(transport.setMaxIncomingBitrate(100000))
		.resolves
		.toBeUndefined();
}, 2000);

test('webRtcTransport.restartIce() succeeds', async () =>
{
	const previousIceUsernameFragment = transport.iceParameters.usernameFragment;
	const previousIcePassword = transport.iceParameters.password;

	await expect(transport.restartIce())
		.resolves
		.toMatchObject(
			{
				usernameFragment : expect.any(String),
				password         : expect.any(String),
				iceLite          : true
			});

	expect(transport.iceParameters.usernameFragment).toBeType('string');
	expect(transport.iceParameters.password).toBeType('string');
	expect(transport.iceParameters.usernameFragment)
		.not.toBe(previousIceUsernameFragment);
	expect(transport.iceParameters.password).not.toBe(previousIcePassword);
}, 2000);

test('transport.enableTraceEvent() succeed', async () =>
{
	await transport.enableTraceEvent([ 'foo', 'probation' ]);
	await expect(transport.dump())
		.resolves
		.toMatchObject({ traceEventTypes: 'probation' });

	await transport.enableTraceEvent([]);
	await expect(transport.dump())
		.resolves
		.toMatchObject({ traceEventTypes: '' });

	await transport.enableTraceEvent([ 'probation', 'FOO', 'bwe', 'BAR' ]);
	await expect(transport.dump())
		.resolves
		.toMatchObject({ traceEventTypes: 'probation,bwe' });

	await transport.enableTraceEvent();
	await expect(transport.dump())
		.resolves
		.toMatchObject({ traceEventTypes: '' });
}, 2000);

test('transport.enableTraceEvent() with wrong arguments rejects with TypeError', async () =>
{
	await expect(transport.enableTraceEvent(123))
		.rejects
		.toThrow(TypeError);

	await expect(transport.enableTraceEvent('probation'))
		.rejects
		.toThrow(TypeError);

	await expect(transport.enableTraceEvent([ 'probation', 123.123 ]))
		.rejects
		.toThrow(TypeError);
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
	const onObserverClose = jest.fn();

	transport.observer.once('close', onObserverClose);
	transport.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport.closed).toBe(true);
	expect(transport.iceState).toBe('closed');
	expect(transport.iceSelectedTuple).toBeUndefined();
	expect(transport.dtlsState).toBe('closed');
	expect(transport.sctpState).toBeUndefined();

	await expect(transport.dump())
		.rejects
		.toThrow(Error);

	await expect(transport.getStats())
		.rejects
		.toThrow(Error);

	await expect(transport.connect({}))
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
	const transport2 = await router2.createWebRtcTransport(
		{
			listenIps  : [ '127.0.0.1' ],
			enableSctp : true
		});
	const onObserverClose = jest.fn();

	transport2.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		transport2.on('routerclose', resolve);
		router2.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport2.closed).toBe(true);
	expect(transport2.iceState).toBe('closed');
	expect(transport2.iceSelectedTuple).toBeUndefined();
	expect(transport2.dtlsState).toBe('closed');
	expect(transport2.sctpState).toBe('closed');
}, 2000);

test('WebRtcTransport emits "routerclose" if Worker is closed', async () =>
{
	const onObserverClose = jest.fn();

	transport.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		transport.on('routerclose', resolve);
		worker.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport.closed).toBe(true);
	expect(transport.iceState).toBe('closed');
	expect(transport.iceSelectedTuple).toBeUndefined();
	expect(transport.dtlsState).toBe('closed');
	expect(transport.sctpState).toBeUndefined();
}, 2000);
