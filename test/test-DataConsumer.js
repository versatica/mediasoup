const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;

expect.extend({ toBeType });

let worker;
let router;
let transport1;
let transport2;
let dataProducer;
let dataConsumer;

const dataProducerParameters =
{
	sctpStreamParameters:
	{
		streamId         : 12345,
		ordered          : false,
		maxPacketLifeTime: 5000
	},
	label   : 'foo',
	protocol: 'bar'
};

beforeAll(async () =>
{
	worker = await createWorker();
	router = await worker.createRouter();
	transport1 = await router.createWebRtcTransport(
		{
			listenIps : [ '127.0.0.1' ],
			enableSctp: true
		});
	transport2 = await router.createPlainRtpTransport(
		{
			listenIp  : '127.0.0.1',
			enableSctp: true
		});
	dataProducer = await transport1.produceData(dataProducerParameters);
});

afterAll(() => worker.close());

test('transport.consumeData() succeeds', async () =>
{
	const onObserverNewDataConsumer = jest.fn();

	transport2.observer.once('newdataconsumer', onObserverNewDataConsumer);

	dataConsumer = await transport2.consumeData(
		{
			dataProducerId: dataProducer.id,
			appData       : { baz: 'LOL' }
		});

	expect(onObserverNewDataConsumer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataConsumer).toHaveBeenCalledWith(dataConsumer);
	expect(dataConsumer.id).toBeType('string');
	expect(dataConsumer.dataProducerId).toBe(dataProducer.id);
	expect(dataConsumer.closed).toBe(false);
	expect(dataConsumer.sctpStreamParameters).toBeType('object');
	expect(dataConsumer.sctpStreamParameters.streamId).toBeType('number');
	expect(dataConsumer.sctpStreamParameters.ordered).toBe(false);
	expect(dataConsumer.sctpStreamParameters.maxPacketLifeTime).toBe(5000);
	expect(dataConsumer.sctpStreamParameters.maxRetransmits).toBe(undefined);
	expect(dataConsumer.label).toBe('foo');
	expect(dataConsumer.protocol).toBe('bar');
	expect(dataConsumer.appData).toEqual({ baz: 'LOL' });

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds: { [dataProducer.id]: [ dataConsumer.id ] },
				mapDataConsumerIdDataProducerId : { [dataConsumer.id]: dataProducer.id }
			});

	await expect(transport2.dump())
		.resolves
		.toMatchObject(
			{
				id             : transport2.id,
				dataProducerIds: [],
				dataConsumerIds: [ dataConsumer.id ]
			});
}, 2000);

test('dataConsumer.dump() succeeds', async () =>
{
	const data = await dataConsumer.dump();

	expect(data.id).toBe(dataConsumer.id);
	expect(data.sctpStreamParameters).toBeType('object');
	expect(data.sctpStreamParameters.streamId)
		.toBe(dataConsumer.sctpStreamParameters.streamId);
	expect(data.sctpStreamParameters.ordered).toBe(false);
	expect(data.sctpStreamParameters.maxPacketLifeTime).toBe(5000);
	expect(data.sctpStreamParameters.maxRetransmits).toBe(undefined);
	expect(data.label).toBe('foo');
	expect(data.protocol).toBe('bar');
}, 2000);

test('dataConsumer.getStats() succeeds', async () =>
{
	await expect(dataConsumer.getStats())
		.resolves
		.toMatchObject(
			[
				{
					type        : 'data-consumer',
					label       : dataConsumer.label,
					protocol    : dataConsumer.protocol,
					messagesSent: 0,
					bytesSent   : 0
				}
			]);
}, 2000);

test('dataConsumer.close() succeeds', async () =>
{
	const onObserverClose = jest.fn();

	dataConsumer.observer.once('close', onObserverClose);
	dataConsumer.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataConsumer.closed).toBe(true);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds: { [dataProducer.id]: [] },
				mapDataConsumerIdDataProducerId : {}
			});

	await expect(transport2.dump())
		.resolves
		.toMatchObject(
			{
				id             : transport2.id,
				dataProducerIds: [],
				dataConsumerIds: []
			});
}, 2000);

test('Consumer methods reject if closed', async () =>
{
	await expect(dataConsumer.dump())
		.rejects
		.toThrow(Error);

	await expect(dataConsumer.getStats())
		.rejects
		.toThrow(Error);
}, 2000);

test('DataConsumer emits "dataproducerclose" if DataProducer is closed', async () =>
{
	dataConsumer = await transport2.consumeData(
		{
			dataProducerId: dataProducer.id
		});

	const onObserverClose = jest.fn();

	dataConsumer.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		dataConsumer.on('dataproducerclose', resolve);
		dataProducer.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataConsumer.closed).toBe(true);
}, 2000);

test('Consumer emits "transportclose" if Transport is closed', async () =>
{
	dataProducer = await transport1.produceData(dataProducerParameters);
	dataConsumer = await transport2.consumeData(
		{
			dataProducerId: dataProducer.id
		});

	const onObserverClose = jest.fn();

	dataConsumer.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		dataConsumer.on('transportclose', resolve);
		transport2.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataConsumer.closed).toBe(true);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds: {},
				mapDataConsumerIdDataProducerId : {}
			});
}, 2000);
