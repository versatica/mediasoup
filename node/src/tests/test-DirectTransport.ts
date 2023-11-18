import * as mediasoup from '../';

let worker: mediasoup.types.Worker;
let router: mediasoup.types.Router;
let transport: mediasoup.types.DirectTransport;

beforeAll(async () =>
{
	worker = await mediasoup.createWorker();
	router = await worker.createRouter();
});

afterAll(() => worker.close());

beforeEach(async () =>
{
	transport = await router.createDirectTransport();
});

afterEach(() => transport.close());

test('router.createDirectTransport() succeeds', async () =>
{
	await expect(router.dump())
		.resolves
		.toMatchObject({ transportIds: [ transport.id ] });

	const onObserverNewTransport = jest.fn();

	router.observer.once('newtransport', onObserverNewTransport);

	// Create a separate transport here.
	const transport1 = await router.createDirectTransport(
		{
			maxMessageSize : 1024,
			appData        : { foo: 'bar' }
		});

	expect(onObserverNewTransport).toHaveBeenCalledTimes(1);
	expect(onObserverNewTransport).toHaveBeenCalledWith(transport1);
	expect(typeof transport1.id).toBe('string');
	expect(transport1.closed).toBe(false);
	expect(transport1.appData).toEqual({ foo: 'bar' });

	const data1 = await transport1.dump();

	expect(data1.id).toBe(transport1.id);
	expect(data1.direct).toBe(true);
	expect(data1.producerIds).toEqual([]);
	expect(data1.consumerIds).toEqual([]);
	expect(data1.dataProducerIds).toEqual([]);
	expect(data1.dataConsumerIds).toEqual([]);
	expect(data1.recvRtpHeaderExtensions).toBeDefined();
	expect(typeof data1.rtpListener).toBe('object');

	transport1.close();
	expect(transport1.closed).toBe(true);
}, 2000);

test('router.createDirectTransport() with wrong arguments rejects with TypeError', async () =>
{
	// @ts-ignore
	await expect(router.createDirectTransport({ maxMessageSize: 'foo' }))
		.rejects
		.toThrow(TypeError);

	await expect(router.createDirectTransport({ maxMessageSize: -2000 }))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('directTransport.getStats() succeeds', async () =>
{
	const data = await transport.getStats();

	expect(Array.isArray(data)).toBe(true);
	expect(data.length).toBe(1);
	expect(data[0].type).toBe('direct-transport');
	expect(data[0].transportId).toBe(transport.id);
	expect(typeof data[0].timestamp).toBe('number');
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
}, 2000);

test('directTransport.connect() succeeds', async () =>
{
	await expect(transport.connect())
		.resolves
		.toBeUndefined();
}, 2000);

test('dataProducer.send() succeeds', async () =>
{
	const transport2 = await router.createDirectTransport();
	const dataProducer = await transport2.produceData(
		{
			label    : 'foo',
			protocol : 'bar',
			appData  : { foo: 'bar' }
		});
	const dataConsumer = await transport2.consumeData(
		{
			dataProducerId : dataProducer.id
		});
	const numMessages = 200;
	const pauseSendingAtMessage = 10;
	const resumeSendingAtMessage = 20;
	const pauseReceivingAtMessage = 40;
	const resumeReceivingAtMessage = 60;
	const expectedReceivedNumMessages =
		numMessages -
		(resumeSendingAtMessage - pauseSendingAtMessage) -
		(resumeReceivingAtMessage - pauseReceivingAtMessage);

	let sentMessageBytes = 0;
	let effectivelySentMessageBytes = 0;
	let recvMessageBytes = 0;
	let numSentMessages = 0;
	let numReceivedMessages = 0;

	// eslint-disable-next-line no-async-promise-executor
	await new Promise<void>(async (resolve, reject) =>
	{
		dataProducer.on('listenererror', (eventName, error) =>
		{
			reject(
				new Error(`dataProducer 'listenererror' [eventName:${eventName}]: ${error}`)
			);
		});

		dataConsumer.on('listenererror', (eventName, error) =>
		{
			reject(
				new Error(`dataConsumer 'listenererror' [eventName:${eventName}]: ${error}`)
			);
		});

		sendNextMessage();

		async function sendNextMessage(): Promise<void>
		{
			const id = ++numSentMessages;
			let ppid;
			let message;

			if (id === pauseSendingAtMessage)
			{
				await dataProducer.pause();
			}
			else if (id === resumeSendingAtMessage)
			{
				await dataProducer.resume();
			}
			else if (id === pauseReceivingAtMessage)
			{
				await dataConsumer.pause();
			}
			else if (id === resumeReceivingAtMessage)
			{
				await dataConsumer.resume();
			}

			// Send string (WebRTC DataChannel string).
			if (id < numMessages / 2)
			{
				message = String(id);
			}
			// Send string (WebRTC DataChannel binary).
			else
			{
				message = Buffer.from(String(id));
			}

			dataProducer.send(message, ppid);

			const messageSize = Buffer.from(message).byteLength;

			sentMessageBytes += messageSize;

			if (!dataProducer.paused && !dataConsumer.paused)
			{
				effectivelySentMessageBytes += messageSize;
			}

			if (id < numMessages)
			{
				sendNextMessage();
			}
		}

		dataConsumer.on('message', (message, ppid) =>
		{
			++numReceivedMessages;

			// message is always a Buffer.
			recvMessageBytes += message.byteLength;

			const id = Number(message.toString('utf8'));

			if (id === numMessages)
			{
				resolve();
			}
			// PPID of WebRTC DataChannel string.
			else if (id < numMessages / 2 && ppid !== 51)
			{
				reject(
					new Error(`ppid in message with id ${id} should be 51 but it is ${ppid}`)
				);
			}
			// PPID of WebRTC DataChannel binary.
			else if (id > numMessages / 2 && ppid !== 53)
			{
				reject(
					new Error(`ppid in message with id ${id} should be 53 but it is ${ppid}`)
				);
			}
		});
	});

	expect(numSentMessages).toBe(numMessages);
	expect(numReceivedMessages).toBe(expectedReceivedNumMessages);
	expect(recvMessageBytes).toBe(effectivelySentMessageBytes);

	await expect(dataProducer.getStats())
		.resolves
		.toMatchObject(
			[
				{
					type             : 'data-producer',
					label            : dataProducer.label,
					protocol         : dataProducer.protocol,
					messagesReceived : numMessages,
					bytesReceived    : sentMessageBytes
				}
			]);

	await expect(dataConsumer.getStats())
		.resolves
		.toMatchObject(
			[
				{
					type         : 'data-consumer',
					label        : dataConsumer.label,
					protocol     : dataConsumer.protocol,
					messagesSent : expectedReceivedNumMessages,
					bytesSent    : recvMessageBytes
				}
			]);
}, 5000);

test('dataProducer.send() with subchannels succeeds', async () =>
{
	const transport2 = await router.createDirectTransport();
	const dataProducer = await transport2.produceData();
	const dataConsumer1 = await transport2.consumeData(
		{
			dataProducerId : dataProducer.id,
			subchannels    : [ 1, 11, 666 ]
		});
	const dataConsumer2 = await transport2.consumeData(
		{
			dataProducerId : dataProducer.id,
			subchannels    : [ 2, 22, 666 ]
		});
	const expectedReceivedNumMessages1 = 7;
	const expectedReceivedNumMessages2 = 5;
	const receivedMessages1: string[] = [];
	const receivedMessages2: string[] = [];

	// eslint-disable-next-line no-async-promise-executor
	await new Promise<void>(async (resolve) =>
	{
		// Must be received by dataConsumer1 and dataConsumer2.
		dataProducer.send(
			'both',
			/* ppid */ undefined,
			/* subchannels */ undefined,
			/* requiredSubchannel */ undefined
		);

		// Must be received by dataConsumer1 and dataConsumer2.
		dataProducer.send(
			'both',
			/* ppid */ undefined,
			/* subchannels */ [ 1, 2 ],
			/* requiredSubchannel */ undefined
		);

		// Must be received by dataConsumer1 and dataConsumer2.
		dataProducer.send(
			'both',
			/* ppid */ undefined,
			/* subchannels */ [ 11, 22, 33 ],
			/* requiredSubchannel */ 666
		);

		// Must not be received by neither dataConsumer1 nor dataConsumer2.
		dataProducer.send(
			'none',
			/* ppid */ undefined,
			/* subchannels */ [ 3 ],
			/* requiredSubchannel */ 666
		);

		// Must not be received by neither dataConsumer1 nor dataConsumer2.
		dataProducer.send(
			'none',
			/* ppid */ undefined,
			/* subchannels */ [ 666 ],
			/* requiredSubchannel */ 3
		);

		// Must be received by dataConsumer1.
		dataProducer.send(
			'dc1',
			/* ppid */ undefined,
			/* subchannels */ [ 1 ],
			/* requiredSubchannel */ undefined
		);

		// Must be received by dataConsumer1.
		dataProducer.send(
			'dc1',
			/* ppid */ undefined,
			/* subchannels */ [ 11 ],
			/* requiredSubchannel */ 1
		);

		// Must be received by dataConsumer1.
		dataProducer.send(
			'dc1',
			/* ppid */ undefined,
			/* subchannels */ [ 666 ],
			/* requiredSubchannel */ 11
		);

		// Must be received by dataConsumer2.
		dataProducer.send(
			'dc2',
			/* ppid */ undefined,
			/* subchannels */ [ 666 ],
			/* requiredSubchannel */ 2
		);

		// Make dataConsumer2 also subscribe to subchannel 1.
		// NOTE: No need to await for this call.
		void dataConsumer2.setSubchannels([ ...dataConsumer2.subchannels, 1 ]);

		// Must be received by dataConsumer1 and dataConsumer2.
		dataProducer.send(
			'both',
			/* ppid */ undefined,
			/* subchannels */ [ 1 ],
			/* requiredSubchannel */ 666
		);

		dataConsumer1.on('message', (message) =>
		{
			receivedMessages1.push(message.toString('utf8'));

			if (
				receivedMessages1.length === expectedReceivedNumMessages1 &&
				receivedMessages2.length === expectedReceivedNumMessages2
			)
			{
				resolve();
			}
		});

		dataConsumer2.on('message', (message) =>
		{
			receivedMessages2.push(message.toString('utf8'));

			if (
				receivedMessages1.length === expectedReceivedNumMessages1 &&
				receivedMessages2.length === expectedReceivedNumMessages2
			)
			{
				resolve();
			}
		});
	});

	expect(receivedMessages1.length).toBe(expectedReceivedNumMessages1);
	expect(receivedMessages2.length).toBe(expectedReceivedNumMessages2);

	for (const message of receivedMessages1)
	{
		expect([ 'both', 'dc1' ].includes(message)).toBe(true);
		expect([ 'dc2' ].includes(message)).toBe(false);
	}

	for (const message of receivedMessages2)
	{
		expect([ 'both', 'dc2' ].includes(message)).toBe(true);
		expect([ 'dc1' ].includes(message)).toBe(false);
	}
}, 5000);

test('DirectTransport methods reject if closed', async () =>
{
	const onObserverClose = jest.fn();

	transport.observer.once('close', onObserverClose);
	transport.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport.closed).toBe(true);

	await expect(transport.dump())
		.rejects
		.toThrow(Error);

	await expect(transport.getStats())
		.rejects
		.toThrow(Error);
}, 2000);

test('DirectTransport emits "routerclose" if Router is closed', async () =>
{
	// We need different Router and DirectTransport instances here.
	const router2 = await worker.createRouter();
	const transport2 = await router2.createDirectTransport();
	const onObserverClose = jest.fn();

	transport2.observer.once('close', onObserverClose);

	await new Promise<void>((resolve) =>
	{
		transport2.on('routerclose', resolve);
		router2.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport2.closed).toBe(true);
}, 2000);

test('DirectTransport emits "routerclose" if Worker is closed', async () =>
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
}, 2000);
