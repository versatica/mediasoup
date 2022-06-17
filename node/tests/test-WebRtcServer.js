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
		.toEqual({ pid: worker.pid, webRtcServerIds: [ webRtcServer.id ], routerIds: [] });

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
				tuples                    : []
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

test('router.createWebRtcTransport() with webRtcServer succeeds', async () =>
{
	worker = await createWorker();

	const router = await worker.createRouter();
	const port1 = await pickPort({ ip: '127.0.0.1', reserveTimeout: 0 });
	const port2 = await pickPort({ type: 'tcp', ip: '127.0.0.1', reserveTimeout: 0 });
	const webRtcServer = await worker.createWebRtcServer(
		{
			listenInfos :
			[
				{ protocol: 'udp', ip: '127.0.0.1', port: port1 },
				{ protocol: 'tcp', ip: '127.0.0.1', port: port2 },
			]
		});

	await expect(router.dump())
		.resolves
		.toMatchObject({ transportIds: [ transport.id ] });

	const onObserverNewTransport = jest.fn();

	router.observer.once('newtransport', onObserverNewTransport);

	// Create a separate transport here.
	const transport = await router.createWebRtcTransport(
		{
			webRtcServer,
			enableTcp : false,
			preferUdp : true,
			appData   : { foo: 'bar' }
		});

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
					{ /*localIceUsernameFragment*/ webRtcTransportId: transport.id }
				],
				tuples : []
			});

	transport.close();
	expect(transport.closed).toBe(true);

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
				tuples                    : []
			});
}, 2000);
