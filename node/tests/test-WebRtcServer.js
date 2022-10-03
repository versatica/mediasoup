const { toBeType } = require('jest-tobetype');
const pickPort = require('pick-port');
const mediasoup = require('../lib/');
const { createWorker } = mediasoup;
const { InvalidStateError } = require('../lib/errors');

expect.extend({ toBeType });

let worker;

beforeEach(() => worker && !worker.closed && worker.close());
afterEach(() => worker && !worker.closed && worker.close());

test('worker.createWebRtcServer() succeeds', async () =>
{
	worker = await createWorker();

	const onObserverNewWebRtcServer = jest.fn();

	worker.observer.once('newwebrtcserver', onObserverNewWebRtcServer);

	const port1 = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const port2 = await pickPort({ type: 'tcp', ip: '127.0.0.1', reserveTimeout: 0 });

	const webRtcServer = await worker.createWebRtcServer(
		{
			listenInfos :
			[
				{
					protocol : 'udp',
					ip       : '127.0.0.1',
					port     : port1
				},
				{
					protocol    : 'tcp',
					ip          : '127.0.0.1',
					announcedIp : '1.2.3.4',
					port        : port2
				}
			],
			appData : { foo: 123 }
		});

	expect(onObserverNewWebRtcServer).toHaveBeenCalledTimes(1);
	expect(onObserverNewWebRtcServer).toHaveBeenCalledWith(webRtcServer);
	expect(webRtcServer.id).toBeType('string');
	expect(webRtcServer.closed).toBe(false);
	expect(webRtcServer.appData).toEqual({ foo: 123 });

	await expect(worker.dump())
		.resolves
		.toEqual(
			{
				pid                    : worker.pid,
				webRtcServerIds        : [ webRtcServer.id ],
				routerIds              : [],
				channelMessageHandlers :
				{
					channelRequestHandlers             : [ webRtcServer.id ],
					payloadChannelRequestHandlers      : [],
					payloadChannelNotificationHandlers : []
				}
			});

	await expect(webRtcServer.dump())
		.resolves
		.toMatchObject(
			{
				id         : webRtcServer.id,
				udpSockets :
				[
					{ ip: '127.0.0.1', port: port1 }
				],
				tcpServers :
				[
					{ ip: '127.0.0.1', port: port2 }
				],
				webRtcTransportIds        : [],
				localIceUsernameFragments : [],
				tupleHashes               : []
			});

	// Private API.
	expect(worker.webRtcServersForTesting.size).toBe(1);

	worker.close();

	expect(webRtcServer.closed).toBe(true);

	// Private API.
	expect(worker.webRtcServersForTesting.size).toBe(0);
}, 2000);

test('worker.createWebRtcServer() without specifying port succeeds', async () =>
{
	worker = await createWorker();

	const onObserverNewWebRtcServer = jest.fn();

	worker.observer.once('newwebrtcserver', onObserverNewWebRtcServer);

	const webRtcServer = await worker.createWebRtcServer(
		{
			listenInfos :
			[
				{
					protocol : 'udp',
					ip       : '127.0.0.1'
				},
				{
					protocol    : 'tcp',
					ip          : '127.0.0.1',
					announcedIp : '1.2.3.4'
				}
			],
			appData : { foo: 123 }
		});

	expect(onObserverNewWebRtcServer).toHaveBeenCalledTimes(1);
	expect(onObserverNewWebRtcServer).toHaveBeenCalledWith(webRtcServer);
	expect(webRtcServer.id).toBeType('string');
	expect(webRtcServer.closed).toBe(false);
	expect(webRtcServer.appData).toEqual({ foo: 123 });

	await expect(worker.dump())
		.resolves
		.toEqual(
			{
				pid                    : worker.pid,
				webRtcServerIds        : [ webRtcServer.id ],
				routerIds              : [],
				channelMessageHandlers :
				{
					channelRequestHandlers             : [ webRtcServer.id ],
					payloadChannelRequestHandlers      : [],
					payloadChannelNotificationHandlers : []
				}
			});

	await expect(webRtcServer.dump())
		.resolves
		.toMatchObject(
			{
				id         : webRtcServer.id,
				udpSockets :
				[
					{ ip: '127.0.0.1', port: expect.any(Number) }
				],
				tcpServers :
				[
					{ ip: '127.0.0.1', port: expect.any(Number) }
				],
				webRtcTransportIds        : [],
				localIceUsernameFragments : [],
				tupleHashes               : []
			});

	// Private API.
	expect(worker.webRtcServersForTesting.size).toBe(1);

	worker.close();

	expect(webRtcServer.closed).toBe(true);

	// Private API.
	expect(worker.webRtcServersForTesting.size).toBe(0);
}, 2000);

test('worker.createWebRtcServer() with wrong arguments rejects with TypeError', async () =>
{
	worker = await createWorker();

	await expect(worker.createWebRtcServer({ }))
		.rejects
		.toThrow(TypeError);

	await expect(worker.createWebRtcServer({ listenInfos: 'NOT-AN-ARRAY' }))
		.rejects
		.toThrow(TypeError);

	await expect(worker.createWebRtcServer({ listenInfos: [ 'NOT-AN-OBJECT' ] }))
		.rejects
		.toThrow(TypeError);

	// Empty listenInfos so should fail.
	await expect(worker.createWebRtcServer({ listenInfos: [] }))
		.rejects
		.toThrow(TypeError);

	worker.close();
}, 2000);

test('worker.createWebRtcServer() with unavailable listenInfos rejects with Error', async () =>
{
	const worker1 = await createWorker();
	const worker2 = await createWorker();
	const port1 = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const port2 = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });

	// Using an unavailable listen IP.
	await expect(worker1.createWebRtcServer(
		{
			listenInfos :
			[
				{
					protocol : 'udp',
					ip       : '127.0.0.1',
					port     : port1
				},
				{
					protocol : 'udp',
					ip       : '1.2.3.4',
					port     : port2
				}
			]
		}))
		.rejects
		.toThrow(Error);

	// Using the same UDP port in two listenInfos.
	await expect(worker1.createWebRtcServer(
		{
			listenInfos :
			[
				{
					protocol : 'udp',
					ip       : '127.0.0.1',
					port     : port1
				},
				{
					protocol    : 'udp',
					ip          : '127.0.0.1',
					announcedIp : '1.2.3.4',
					port        : port1
				}
			]
		}))
		.rejects
		.toThrow(Error);

	await worker1.createWebRtcServer(
		{
			listenInfos :
			[
				{
					protocol : 'udp',
					ip       : '127.0.0.1',
					port     : port1
				}
			]
		});

	// Using the same UDP port in a second Worker.
	await expect(worker2.createWebRtcServer(
		{
			listenInfos :
			[
				{
					protocol : 'udp',
					ip       : '127.0.0.1',
					port     : port1
				}
			]
		}))
		.rejects
		.toThrow(Error);

	worker1.close();
	worker2.close();
}, 2000);

test('worker.createWebRtcServer() rejects with InvalidStateError if Worker is closed', async () =>
{
	worker = await createWorker();

	worker.close();

	const port = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });

	await expect(worker.createWebRtcServer(
		{
			listenInfos : [ { protocol: 'udp', ip: '127.0.0.1', port } ]
		}))
		.rejects
		.toThrow(InvalidStateError);
}, 2000);

test('webRtcServer.close() succeeds', async () =>
{
	worker = await createWorker();

	const port = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const webRtcServer = await worker.createWebRtcServer(
		{
			listenInfos : [ { protocol: 'udp', ip: '127.0.0.1', port } ]
		});
	const onObserverClose = jest.fn();

	webRtcServer.observer.once('close', onObserverClose);
	webRtcServer.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(webRtcServer.closed).toBe(true);
}, 2000);

test('WebRtcServer emits "workerclose" if Worker is closed', async () =>
{
	worker = await createWorker();

	const port = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const webRtcServer = await worker.createWebRtcServer(
		{
			listenInfos : [ { protocol: 'tcp', ip: '127.0.0.1', port } ]
		});
	const onObserverClose = jest.fn();

	webRtcServer.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		webRtcServer.on('workerclose', resolve);
		worker.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(webRtcServer.closed).toBe(true);
}, 2000);

test('router.createWebRtcTransport() with webRtcServer succeeds and transport is closed', async () =>
{
	worker = await createWorker();

	const port1 = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const port2 = await pickPort({ type: 'tcp', ip: '127.0.0.1', reserveTimeout: 0 });
	const webRtcServer = await worker.createWebRtcServer(
		{
			listenInfos :
			[
				{ protocol: 'udp', ip: '127.0.0.1', port: port1 },
				{ protocol: 'tcp', ip: '127.0.0.1', port: port2 }
			]
		});

	const onObserverWebRtcTransportHandled = jest.fn();
	const onObserverWebRtcTransportUnhandled = jest.fn();

	webRtcServer.observer.once('webrtctransporthandled', onObserverWebRtcTransportHandled);
	webRtcServer.observer.once('webrtctransportunhandled', onObserverWebRtcTransportUnhandled);

	const router = await worker.createRouter();

	const onObserverNewTransport = jest.fn();

	router.observer.once('newtransport', onObserverNewTransport);

	const transport = await router.createWebRtcTransport(
		{
			webRtcServer,
			enableTcp : false,
			appData   : { foo: 'bar' }
		});

	await expect(router.dump())
		.resolves
		.toMatchObject({ transportIds: [ transport.id ] });

	expect(onObserverWebRtcTransportHandled).toHaveBeenCalledTimes(1);
	expect(onObserverWebRtcTransportHandled).toHaveBeenCalledWith(transport);
	expect(onObserverNewTransport).toHaveBeenCalledTimes(1);
	expect(onObserverNewTransport).toHaveBeenCalledWith(transport);
	expect(transport.id).toBeType('string');
	expect(transport.closed).toBe(false);
	expect(transport.appData).toEqual({ foo: 'bar' });

	const iceCandidates = transport.iceCandidates;

	expect(iceCandidates.length).toBe(1);
	expect(iceCandidates[0].ip).toBe('127.0.0.1');
	expect(iceCandidates[0].port).toBe(port1);
	expect(iceCandidates[0].protocol).toBe('udp');
	expect(iceCandidates[0].type).toBe('host');
	expect(iceCandidates[0].tcpType).toBeUndefined();

	expect(transport.iceState).toBe('new');
	expect(transport.iceSelectedTuple).toBeUndefined();

	expect(webRtcServer.webRtcTransportsForTesting.size).toBe(1);
	expect(router.transportsForTesting.size).toBe(1);

	await expect(webRtcServer.dump())
		.resolves
		.toMatchObject(
			{
				id         : webRtcServer.id,
				udpSockets :
				[
					{ ip: '127.0.0.1', port: port1 }
				],
				tcpServers :
				[
					{ ip: '127.0.0.1', port: port2 }
				],
				webRtcTransportIds        : [ transport.id ],
				localIceUsernameFragments :
				[
					{ /* localIceUsernameFragment: xxx, */ webRtcTransportId: transport.id }
				],
				tupleHashes : []
			});

	transport.close();

	expect(transport.closed).toBe(true);
	expect(onObserverWebRtcTransportUnhandled).toHaveBeenCalledTimes(1);
	expect(onObserverWebRtcTransportUnhandled).toHaveBeenCalledWith(transport);
	expect(webRtcServer.webRtcTransportsForTesting.size).toBe(0);
	expect(router.transportsForTesting.size).toBe(0);

	await expect(webRtcServer.dump())
		.resolves
		.toMatchObject(
			{
				id         : webRtcServer.id,
				udpSockets :
				[
					{ ip: '127.0.0.1', port: port1 }
				],
				tcpServers :
				[
					{ ip: '127.0.0.1', port: port2 }
				],
				webRtcTransportIds        : [],
				localIceUsernameFragments : [],
				tupleHashes               : []
			});
}, 2000);

test('router.createWebRtcTransport() with webRtcServer succeeds and webRtcServer is closed', async () =>
{
	worker = await createWorker();

	const port1 = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const port2 = await pickPort({ type: 'tcp', ip: '127.0.0.1', reserveTimeout: 0 });
	const webRtcServer = await worker.createWebRtcServer(
		{
			listenInfos :
			[
				{ protocol: 'udp', ip: '127.0.0.1', port: port1 },
				{ protocol: 'tcp', ip: '127.0.0.1', port: port2 }
			]
		});

	const onObserverWebRtcTransportHandled = jest.fn();
	const onObserverWebRtcTransportUnhandled = jest.fn();

	webRtcServer.observer.once('webrtctransporthandled', onObserverWebRtcTransportHandled);
	webRtcServer.observer.once('webrtctransportunhandled', onObserverWebRtcTransportUnhandled);

	const router = await worker.createRouter();
	const transport = await router.createWebRtcTransport(
		{
			webRtcServer,
			enableUdp : false,
			enableTcp : true,
			appData   : { foo: 'bar' }
		});

	expect(onObserverWebRtcTransportHandled).toHaveBeenCalledTimes(1);
	expect(onObserverWebRtcTransportHandled).toHaveBeenCalledWith(transport);

	await expect(router.dump())
		.resolves
		.toMatchObject({ transportIds: [ transport.id ] });

	expect(transport.id).toBeType('string');
	expect(transport.closed).toBe(false);
	expect(transport.appData).toEqual({ foo: 'bar' });

	const iceCandidates = transport.iceCandidates;

	expect(iceCandidates.length).toBe(1);
	expect(iceCandidates[0].ip).toBe('127.0.0.1');
	expect(iceCandidates[0].port).toBe(port2);
	expect(iceCandidates[0].protocol).toBe('tcp');
	expect(iceCandidates[0].type).toBe('host');
	expect(iceCandidates[0].tcpType).toBe('passive');

	expect(transport.iceState).toBe('new');
	expect(transport.iceSelectedTuple).toBeUndefined();

	expect(webRtcServer.webRtcTransportsForTesting.size).toBe(1);
	expect(router.transportsForTesting.size).toBe(1);

	await expect(webRtcServer.dump())
		.resolves
		.toMatchObject(
			{
				id         : webRtcServer.id,
				udpSockets :
				[
					{ ip: '127.0.0.1', port: port1 }
				],
				tcpServers :
				[
					{ ip: '127.0.0.1', port: port2 }
				],
				webRtcTransportIds        : [ transport.id ],
				localIceUsernameFragments :
				[
					{ /* localIceUsernameFragment: xxx, */ webRtcTransportId: transport.id }
				],
				tupleHashes : []
			});

	// Let's restart ICE in the transport so it should add a new entry in
	// localIceUsernameFragments in the WebRtcServer.
	await transport.restartIce();

	await expect(webRtcServer.dump())
		.resolves
		.toMatchObject(
			{
				id         : webRtcServer.id,
				udpSockets :
				[
					{ ip: '127.0.0.1', port: port1 }
				],
				tcpServers :
				[
					{ ip: '127.0.0.1', port: port2 }
				],
				webRtcTransportIds        : [ transport.id ],
				localIceUsernameFragments :
				[
					{ /* localIceUsernameFragment: xxx, */ webRtcTransportId: transport.id },
					{ /* localIceUsernameFragment: yyy, */ webRtcTransportId: transport.id }
				],
				tupleHashes : []
			});

	const onObserverClose = jest.fn();

	webRtcServer.observer.once('close', onObserverClose);

	const onListenServerClose = jest.fn();

	transport.once('listenserverclose', onListenServerClose);

	webRtcServer.close();

	expect(webRtcServer.closed).toBe(true);
	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(onListenServerClose).toHaveBeenCalledTimes(1);
	expect(onObserverWebRtcTransportUnhandled).toHaveBeenCalledTimes(1);
	expect(onObserverWebRtcTransportUnhandled).toHaveBeenCalledWith(transport);
	expect(transport.closed).toBe(true);
	expect(webRtcServer.webRtcTransportsForTesting.size).toBe(0);
	expect(router.transportsForTesting.size).toBe(0);

	await expect(worker.dump())
		.resolves
		.toEqual(
			{
				pid                    : worker.pid,
				webRtcServerIds        : [],
				routerIds              : [ router.id ],
				channelMessageHandlers :
				{
					channelRequestHandlers             : [ router.id ],
					payloadChannelRequestHandlers      : [],
					payloadChannelNotificationHandlers : []
				}
			});

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				id           : router.id,
				transportIds : []
			});
}, 2000);
