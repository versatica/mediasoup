const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;

expect.extend({ toBeType });

let worker;
let router;
let transport1;
let transport2;
let transport3;
let dataProducer;
let dataConsumer1;
let dataConsumer2;

const dataProducerParameters =
{
	sctpStreamParameters :
	{
		streamId          : 12345,
		ordered           : false,
		maxPacketLifeTime : 5000
	},
	label    : 'foo',
	protocol : 'bar'
};

beforeAll(async () =>
{
	worker = await createWorker();
	router = await worker.createRouter();
	transport1 = await router.createWebRtcTransport(
		{
			listenIps  : [ '127.0.0.1' ],
			enableSctp : true
		});
	transport2 = await router.createPlainTransport(
		{
			listenIp   : '127.0.0.1',
			enableSctp : true
		});
	transport3 = await router.createDirectTransport();
	dataProducer = await transport1.produceData(dataProducerParameters);
});

afterAll(() => worker.close());

test('transport.consumeData() succeeds', async () =>
{
	const onObserverNewDataConsumer = jest.fn();

	transport2.observer.once('newdataconsumer', onObserverNewDataConsumer);

	dataConsumer1 = await transport2.consumeData(
		{
			dataProducerId    : dataProducer.id,
			maxPacketLifeTime : 4000,
			appData           : { baz: 'LOL' }
		});

	expect(onObserverNewDataConsumer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataConsumer).toHaveBeenCalledWith(dataConsumer1);
	expect(dataConsumer1.id).toBeType('string');
	expect(dataConsumer1.dataProducerId).toBe(dataProducer.id);
	expect(dataConsumer1.closed).toBe(false);
	expect(dataConsumer1.type).toBe('sctp');
	expect(dataConsumer1.sctpStreamParameters).toBeType('object');
	expect(dataConsumer1.sctpStreamParameters.streamId).toBeType('number');
	expect(dataConsumer1.sctpStreamParameters.ordered).toBe(false);
	expect(dataConsumer1.sctpStreamParameters.maxPacketLifeTime).toBe(4000);
	expect(dataConsumer1.sctpStreamParameters.maxRetransmits).toBeUndefined();
	expect(dataConsumer1.label).toBe('foo');
	expect(dataConsumer1.protocol).toBe('bar');
	expect(dataConsumer1.appData).toEqual({ baz: 'LOL' });

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds : { [dataProducer.id]: [ dataConsumer1.id ] },
				mapDataConsumerIdDataProducerId  : { [dataConsumer1.id]: dataProducer.id }
			});

	await expect(transport2.dump())
		.resolves
		.toMatchObject(
			{
				id              : transport2.id,
				dataProducerIds : [],
				dataConsumerIds : [ dataConsumer1.id ]
			});
}, 2000);

test('dataConsumer.dump() succeeds', async () =>
{
	const data = await dataConsumer1.dump();

	expect(data.id).toBe(dataConsumer1.id);
	expect(data.dataProducerId).toBe(dataConsumer1.dataProducerId);
	expect(data.type).toBe('sctp');
	expect(data.sctpStreamParameters).toBeType('object');
	expect(data.sctpStreamParameters.streamId)
		.toBe(dataConsumer1.sctpStreamParameters.streamId);
	expect(data.sctpStreamParameters.ordered).toBe(false);
	expect(data.sctpStreamParameters.maxPacketLifeTime).toBe(4000);
	expect(data.sctpStreamParameters.maxRetransmits).toBeUndefined();
	expect(data.label).toBe('foo');
	expect(data.protocol).toBe('bar');
}, 2000);

test('dataConsumer.getStats() succeeds', async () =>
{
	await expect(dataConsumer1.getStats())
		.resolves
		.toMatchObject(
			[
				{
					type         : 'data-consumer',
					label        : dataConsumer1.label,
					protocol     : dataConsumer1.protocol,
					messagesSent : 0,
					bytesSent    : 0
				}
			]);
}, 2000);

test('transport.consumeData() on a DirectTransport succeeds', async () =>
{
	const onObserverNewDataConsumer = jest.fn();

	transport3.observer.once('newdataconsumer', onObserverNewDataConsumer);

	dataConsumer2 = await transport3.consumeData(
		{
			dataProducerId : dataProducer.id,
			appData        : { hehe: 'HEHE' }
		});

	expect(onObserverNewDataConsumer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataConsumer).toHaveBeenCalledWith(dataConsumer2);
	expect(dataConsumer2.id).toBeType('string');
	expect(dataConsumer2.dataProducerId).toBe(dataProducer.id);
	expect(dataConsumer2.closed).toBe(false);
	expect(dataConsumer2.type).toBe('direct');
	expect(dataConsumer2.sctpStreamParameters).toBeUndefined();
	expect(dataConsumer2.label).toBe('foo');
	expect(dataConsumer2.protocol).toBe('bar');
	expect(dataConsumer2.appData).toEqual({ hehe: 'HEHE' });

	await expect(transport3.dump())
		.resolves
		.toMatchObject(
			{
				id              : transport3.id,
				dataProducerIds : [],
				dataConsumerIds : [ dataConsumer2.id ]
			});
}, 2000);

test('dataConsumer.dump() on a DirectTransport succeeds', async () =>
{
	const data = await dataConsumer2.dump();

	expect(data.id).toBe(dataConsumer2.id);
	expect(data.dataProducerId).toBe(dataConsumer2.dataProducerId);
	expect(data.type).toBe('direct');
	expect(data.sctpStreamParameters).toBeUndefined();
	expect(data.label).toBe('foo');
	expect(data.protocol).toBe('bar');
}, 2000);

test('dataConsumer.getStats() on a DirectTransport succeeds', async () =>
{
	await expect(dataConsumer2.getStats())
		.resolves
		.toMatchObject(
			[
				{
					type         : 'data-consumer',
					label        : dataConsumer2.label,
					protocol     : dataConsumer2.protocol,
					messagesSent : 0,
					bytesSent    : 0
				}
			]);
}, 2000);

test('dataConsumer.close() succeeds', async () =>
{
	const onObserverClose = jest.fn();

	dataConsumer1.observer.once('close', onObserverClose);
	dataConsumer1.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataConsumer1.closed).toBe(true);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds : { [dataProducer.id]: [ dataConsumer2.id ] },
				mapDataConsumerIdDataProducerId  : { [dataConsumer2.id]: dataProducer.id }
			});

	await expect(transport2.dump())
		.resolves
		.toMatchObject(
			{
				id              : transport2.id,
				dataProducerIds : [],
				dataConsumerIds : []
			});
}, 2000);

test('Consumer methods reject if closed', async () =>
{
	await expect(dataConsumer1.dump())
		.rejects
		.toThrow(Error);

	await expect(dataConsumer1.getStats())
		.rejects
		.toThrow(Error);
}, 2000);

test('DataConsumer emits "dataproducerclose" if DataProducer is closed', async () =>
{
	dataConsumer1 = await transport2.consumeData(
		{
			dataProducerId : dataProducer.id
		});

	const onObserverClose = jest.fn();

	dataConsumer1.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		dataConsumer1.on('dataproducerclose', resolve);
		dataProducer.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataConsumer1.closed).toBe(true);
}, 2000);

test('DataConsumer emits "transportclose" if Transport is closed', async () =>
{
	dataProducer = await transport1.produceData(dataProducerParameters);
	dataConsumer1 = await transport2.consumeData(
		{
			dataProducerId : dataProducer.id
		});

	const onObserverClose = jest.fn();

	dataConsumer1.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		dataConsumer1.on('transportclose', resolve);
		transport2.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataConsumer1.closed).toBe(true);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds : {},
				mapDataConsumerIdDataProducerId  : {}
			});
}, 2000);
