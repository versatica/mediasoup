import * as mediasoup from '../';

const { createWorker } = mediasoup;

let worker: mediasoup.types.Worker;
let router: mediasoup.types.Router;
let transport: mediasoup.types.DirectTransport;

beforeAll(async () =>
{
	worker = await createWorker();
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
	expect(typeof data1.recvRtpHeaderExtensions).toBe('object');
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
	let sentMessageBytes = 0;
	let recvMessageBytes = 0;
	let lastSentMessageId = 0;
	let lastRecvMessageId = 0;

	await new Promise<void>((resolve) =>
	{
		// Send messages over the sctpSendStream created above.
		const interval = setInterval(() =>
		{
			const id = ++lastSentMessageId;
			let ppid;
			let message;

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
			sentMessageBytes += Buffer.from(message).byteLength;

			if (id === numMessages)
			{
				clearInterval(interval);
			}
		}, 0);

		dataConsumer.on('message', (message, ppid) =>
		{
			// message is always a Buffer.
			recvMessageBytes += message.byteLength;

			const id = Number(message.toString('utf8'));

			if (id === numMessages)
			{
				clearInterval(interval);
				resolve();
			}

			if (id < numMessages / 2)
			{
				expect(ppid).toBe(51); // PPID of WebRTC DataChannel string.
			}
			else
			{
				expect(ppid).toBe(53); // PPID of WebRTC DataChannel binary.
			}

			expect(id).toBe(++lastRecvMessageId);
		});
	});

	expect(lastSentMessageId).toBe(numMessages);
	expect(lastRecvMessageId).toBe(numMessages);
	expect(recvMessageBytes).toBe(sentMessageBytes);

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
					messagesSent : numMessages,
					bytesSent    : recvMessageBytes
				}
			]);
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
