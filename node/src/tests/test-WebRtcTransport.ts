// @ts-ignore
import * as pickPort from 'pick-port';
import * as flatbuffers from 'flatbuffers';
import * as mediasoup from '../';
import { Notification, Body as NotificationBody, Event } from '../fbs/notification';
import * as FbsTransport from '../fbs/transport';
import * as FbsWebRtcTransport from '../fbs/web-rtc-transport';
import { serializeProtocol, TransportTuple } from '../Transport';

let worker: mediasoup.types.Worker;
let router: mediasoup.types.Router;
let transport: mediasoup.types.WebRtcTransport;

const mediaCodecs: mediasoup.types.RtpCodecCapability[] =
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
	worker = await mediasoup.createWorker();
	router = await worker.createRouter({ mediaCodecs });
});

afterAll(() => worker.close());

beforeEach(async () =>
{
	transport = await router.createWebRtcTransport(
		{
			listenInfos : [ { protocol: 'udp', ip: '127.0.0.1', announcedIp: '9.9.9.1' } ]
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
			listenInfos :
			[
				{ protocol: 'udp', ip: '127.0.0.1', announcedIp: '9.9.9.1' },
				{ protocol: 'tcp', ip: '127.0.0.1', announcedIp: '9.9.9.1' },
				{ protocol: 'udp', ip: '0.0.0.0', announcedIp: '9.9.9.2' },
				{ protocol: 'tcp', ip: '0.0.0.0', announcedIp: '9.9.9.2' },
				{ protocol: 'udp', ip: '127.0.0.1', announcedIp: undefined },
				{ protocol: 'tcp', ip: '127.0.0.1', announcedIp: undefined }
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
	expect(typeof transport1.id).toBe('string');
	expect(transport1.closed).toBe(false);
	expect(transport1.appData).toEqual({ foo: 'bar' });
	expect(transport1.iceRole).toBe('controlled');
	expect(typeof transport1.iceParameters).toBe('object');
	expect(transport1.iceParameters.iceLite).toBe(true);
	expect(typeof transport1.iceParameters.usernameFragment).toBe('string');
	expect(typeof transport1.iceParameters.password).toBe('string');
	expect(transport1.sctpParameters).toMatchObject(
		{
			port           : 5000,
			OS             : 2048,
			MIS            : 2048,
			maxMessageSize : 1000000
		});
	expect(Array.isArray(transport1.iceCandidates)).toBe(true);
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
	expect(iceCandidates[1].priority).toBeGreaterThan(iceCandidates[2].priority);
	expect(iceCandidates[2].priority).toBeGreaterThan(iceCandidates[3].priority);
	expect(iceCandidates[3].priority).toBeGreaterThan(iceCandidates[4].priority);
	expect(iceCandidates[4].priority).toBeGreaterThan(iceCandidates[5].priority);

	expect(transport1.iceState).toBe('new');
	expect(transport1.iceSelectedTuple).toBeUndefined();
	expect(typeof transport1.dtlsParameters).toBe('object');
	expect(Array.isArray(transport1.dtlsParameters.fingerprints)).toBe(true);
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
	expect(data1.recvRtpHeaderExtensions).toBeDefined();
	expect(typeof data1.rtpListener).toBe('object');

	transport1.close();
	expect(transport1.closed).toBe(true);

	const anotherTransport = await router.createWebRtcTransport(
		{
			listenInfos : [ { protocol: 'udp', ip: '127.0.0.1' } ]
		});

	expect(typeof anotherTransport).toBe('object');
}, 2000);

test('router.createWebRtcTransport() with wrong arguments rejects with TypeError', async () =>
{
	// @ts-ignore
	await expect(router.createWebRtcTransport({}))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(router.createWebRtcTransport({ listenIps: [ 123 ] }))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(router.createWebRtcTransport({ listenInfos: '127.0.0.1' }))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(router.createWebRtcTransport({ listenIps: '127.0.0.1' }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ],
			// @ts-ignore
			appData   : 'NOT-AN-OBJECT'
		}))
		.rejects
		.toThrow(TypeError);

	await expect(router.createWebRtcTransport(
		{
			listenIps      : [ '127.0.0.1' ],
			enableSctp     : true,
			// @ts-ignore
			numSctpStreams : 'foo'
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('router.createWebRtcTransport() with non bindable IP rejects with Error', async () =>
{
	await expect(router.createWebRtcTransport(
		{
			listenInfos : [ { protocol: 'udp', ip: '8.8.8.8' } ]
		}))
		.rejects
		.toThrow(Error);
}, 2000);

test('webRtcTransport.getStats() succeeds', async () =>
{
	const data = await transport.getStats();

	expect(Array.isArray(data)).toBe(true);
	expect(data.length).toBe(1);
	expect(data[0].type).toBe('webrtc-transport');
	expect(data[0].transportId).toBe(transport.id);
	expect(typeof data[0].timestamp).toBe('number');
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
}, 2000);

test('webRtcTransport.connect() succeeds', async () =>
{
	const dtlsRemoteParameters: mediasoup.types.DtlsParameters =
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

/**
 * When are we going to rely on the type system in the API?
 * We are testing invalid type values which adds extra checks in the code that should be
 * simply guarded by the type system.
 */
test('webRtcTransport.connect() with wrong arguments rejects with TypeError', async () =>
{
	let dtlsRemoteParameters: mediasoup.types.DtlsParameters;

	// @ts-ignore
	await expect(transport.connect({}))
		.rejects
		.toThrow(TypeError);

	dtlsRemoteParameters =
	{
		fingerprints :
		[
			{
				// @ts-ignore.
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
		// @ts-ignore
		role : 'chicken'
	};

	await expect(transport.connect({ dtlsParameters: dtlsRemoteParameters }))
		.rejects
		.toThrow(TypeError);

	dtlsRemoteParameters =
	{
		fingerprints : [],
		role         : 'client'
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
	await expect(transport.setMaxIncomingBitrate(1000000))
		.resolves
		.toBeUndefined();

	// Remove limit.
	await expect(transport.setMaxIncomingBitrate(0))
		.resolves
		.toBeUndefined();
}, 2000);

test('webRtcTransport.setMaxOutgoingBitrate() succeeds', async () =>
{
	await expect(transport.setMaxOutgoingBitrate(2000000))
		.resolves
		.toBeUndefined();

	// Remove limit.
	await expect(transport.setMaxOutgoingBitrate(0))
		.resolves
		.toBeUndefined();
}, 2000);

test('webRtcTransport.setMinOutgoingBitrate() succeeds', async () =>
{
	await expect(transport.setMinOutgoingBitrate(100000))
		.resolves
		.toBeUndefined();

	// Remove limit.
	await expect(transport.setMinOutgoingBitrate(0))
		.resolves
		.toBeUndefined();
}, 2000);

test('webRtcTransport.setMaxOutgoingBitrate() fails if value is lower than current min limit', async () =>
{
	await expect(transport.setMinOutgoingBitrate(3000000))
		.resolves
		.toBeUndefined();

	await expect(transport.setMaxOutgoingBitrate(2000000))
		.rejects
		.toThrow(Error);

	// Remove limit.
	await expect(transport.setMinOutgoingBitrate(0))
		.resolves
		.toBeUndefined();
}, 2000);

test('webRtcTransport.setMinOutgoingBitrate() fails if value is higher than current max limit', async () =>
{
	await expect(transport.setMaxOutgoingBitrate(2000000))
		.resolves
		.toBeUndefined();

	await expect(transport.setMinOutgoingBitrate(3000000))
		.rejects
		.toThrow(Error);

	// Remove limit.
	await expect(transport.setMaxOutgoingBitrate(0))
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

	expect(typeof transport.iceParameters.usernameFragment).toBe('string');
	expect(typeof transport.iceParameters.password).toBe('string');
	expect(transport.iceParameters.usernameFragment)
		.not.toBe(previousIceUsernameFragment);
	expect(transport.iceParameters.password).not.toBe(previousIcePassword);
}, 2000);

test('transport.enableTraceEvent() succeed', async () =>
{
	// @ts-ignore
	await transport.enableTraceEvent([ 'foo', 'probation' ]);
	await expect(transport.dump())
		.resolves
		.toMatchObject({ traceEventTypes: [ 'probation' ] });

	await transport.enableTraceEvent([]);
	await expect(transport.dump())
		.resolves
		.toMatchObject({ traceEventTypes: [] });

	// @ts-ignore
	await transport.enableTraceEvent([ 'probation', 'FOO', 'bwe', 'BAR' ]);
	await expect(transport.dump())
		.resolves
		.toMatchObject({ traceEventTypes: [ 'probation', 'bwe' ] });

	await transport.enableTraceEvent();
	await expect(transport.dump())
		.resolves
		.toMatchObject({ traceEventTypes: [] });
}, 2000);

test('transport.enableTraceEvent() with wrong arguments rejects with TypeError', async () =>
{
	// @ts-ignore
	await expect(transport.enableTraceEvent(123))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(transport.enableTraceEvent('probation'))
		.rejects
		.toThrow(TypeError);

	// @ts-ignore
	await expect(transport.enableTraceEvent([ 'probation', 123.123 ]))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('WebRtcTransport events succeed', async () =>
{
	// Private API.
	const channel = transport.channelForTesting;
	const onIceStateChange = jest.fn();

	transport.on('icestatechange', onIceStateChange);

	// Simulate a 'iceselectedtuplechange' notification coming through the channel.
	const builder = new flatbuffers.Builder();
	const iceStateChangeNotification = new FbsWebRtcTransport.IceStateChangeNotificationT(
		FbsWebRtcTransport.IceState.COMPLETED);

	let notificationOffset = Notification.createNotification(
		builder,
		builder.createString(transport.id),
		Event.WEBRTCTRANSPORT_ICE_STATE_CHANGE,
		NotificationBody.WebRtcTransport_IceStateChangeNotification,
		iceStateChangeNotification.pack(builder)
	);

	builder.finish(notificationOffset);

	let notification = Notification.getRootAsNotification(
		new flatbuffers.ByteBuffer(builder.asUint8Array()));

	channel.emit(transport.id, Event.WEBRTCTRANSPORT_ICE_STATE_CHANGE, notification);

	expect(onIceStateChange).toHaveBeenCalledTimes(1);
	expect(onIceStateChange).toHaveBeenCalledWith('completed');
	expect(transport.iceState).toBe('completed');

	builder.clear();

	const onIceSelectedTuple = jest.fn();
	const iceSelectedTuple: TransportTuple =
	{
		localIp    : '1.1.1.1',
		localPort  : 1111,
		remoteIp   : '2.2.2.2',
		remotePort : 2222,
		protocol   : 'udp'
	};

	transport.on('iceselectedtuplechange', onIceSelectedTuple);

	// Simulate a 'icestatechange' notification coming through the channel.
	const iceSelectedTupleChangeNotification =
		new FbsWebRtcTransport.IceSelectedTupleChangeNotificationT(
			new FbsTransport.TupleT(
				iceSelectedTuple.localIp,
				iceSelectedTuple.localPort,
				iceSelectedTuple.remoteIp,
				iceSelectedTuple.remotePort,
				serializeProtocol(iceSelectedTuple.protocol))
		);

	notificationOffset = Notification.createNotification(
		builder,
		builder.createString(transport.id),
		Event.WEBRTCTRANSPORT_ICE_SELECTED_TUPLE_CHANGE,
		NotificationBody.WebRtcTransport_IceSelectedTupleChangeNotification,
		iceSelectedTupleChangeNotification.pack(builder)
	);

	builder.finish(notificationOffset);

	notification = Notification.getRootAsNotification(
		new flatbuffers.ByteBuffer(builder.asUint8Array()));

	channel.emit(
		transport.id, Event.WEBRTCTRANSPORT_ICE_SELECTED_TUPLE_CHANGE, notification);

	expect(onIceSelectedTuple).toHaveBeenCalledTimes(1);
	expect(onIceSelectedTuple).toHaveBeenCalledWith(iceSelectedTuple);
	expect(transport.iceSelectedTuple).toEqual(iceSelectedTuple);

	builder.clear();

	const onDtlsStateChange = jest.fn();

	transport.on('dtlsstatechange', onDtlsStateChange);

	// Simulate a 'dtlsstatechange' notification coming through the channel.
	const dtlsStateChangeNotification = new FbsWebRtcTransport.DtlsStateChangeNotificationT(
		FbsWebRtcTransport.DtlsState.CONNECTING);

	notificationOffset = Notification.createNotification(
		builder,
		builder.createString(transport.id),
		Event.WEBRTCTRANSPORT_DTLS_STATE_CHANGE,
		NotificationBody.WebRtcTransport_DtlsStateChangeNotification,
		dtlsStateChangeNotification.pack(builder)
	);

	builder.finish(notificationOffset);

	notification = Notification.getRootAsNotification(
		new flatbuffers.ByteBuffer(builder.asUint8Array()));

	channel.emit(transport.id, Event.WEBRTCTRANSPORT_DTLS_STATE_CHANGE, notification);

	expect(onDtlsStateChange).toHaveBeenCalledTimes(1);
	expect(onDtlsStateChange).toHaveBeenCalledWith('connecting');
	expect(transport.dtlsState).toBe('connecting');
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

	// @ts-ignore
	await expect(transport.connect({}))
		.rejects
		.toThrow(Error);

	await expect(transport.setMaxIncomingBitrate(200000))
		.rejects
		.toThrow(Error);

	await expect(transport.setMaxOutgoingBitrate(200000))
		.rejects
		.toThrow(Error);

	await expect(transport.setMinOutgoingBitrate(100000))
		.rejects
		.toThrow(Error);

	await expect(transport.restartIce())
		.rejects
		.toThrow(Error);
}, 2000);

test('router.createWebRtcTransport() with fixed port succeeds', async () =>
{

	const port = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const webRtcTransport = await router.createWebRtcTransport(
		{
			listenInfos : [ { protocol: 'udp', ip: '127.0.0.1', port } ]
		});

	expect(webRtcTransport.iceCandidates[0].port).toEqual(port);

	webRtcTransport.close();
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

	await new Promise<void>((resolve) =>
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

	await new Promise<void>((resolve) =>
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
