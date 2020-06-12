const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;

expect.extend({ toBeType });

let worker;
let router;
let transport1;
let transport2;
let dataProducer1;
let dataProducer2;

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
});

afterAll(() => worker.close());

test('transport1.produceData() succeeds', async () =>
{
	const onObserverNewDataProducer = jest.fn();

	transport1.observer.once('newdataproducer', onObserverNewDataProducer);

	dataProducer1 = await transport1.produceData(
		{
			sctpStreamParameters :
			{
				streamId : 666
			},
			label    : 'foo',
			protocol : 'bar',
			appData  : { foo: 1, bar: '2' }
		});

	expect(onObserverNewDataProducer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataProducer).toHaveBeenCalledWith(dataProducer1);
	expect(dataProducer1.id).toBeType('string');
	expect(dataProducer1.closed).toBe(false);
	expect(dataProducer1.type).toBe('sctp');
	expect(dataProducer1.sctpStreamParameters).toBeType('object');
	expect(dataProducer1.sctpStreamParameters.streamId).toBe(666);
	expect(dataProducer1.sctpStreamParameters.ordered).toBe(true);
	expect(dataProducer1.sctpStreamParameters.maxPacketLifeTime).toBeUndefined();
	expect(dataProducer1.sctpStreamParameters.maxRetransmits).toBeUndefined();
	expect(dataProducer1.label).toBe('foo');
	expect(dataProducer1.protocol).toBe('bar');
	expect(dataProducer1.appData).toEqual({ foo: 1, bar: '2' });

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds : { [dataProducer1.id]: [] },
				mapDataConsumerIdDataProducerId  : {}
			});

	await expect(transport1.dump())
		.resolves
		.toMatchObject(
			{
				id              : transport1.id,
				dataProducerIds : [ dataProducer1.id ],
				dataConsumerIds : []
			});
}, 2000);

test('transport2.produceData() succeeds', async () =>
{
	const onObserverNewDataProducer = jest.fn();

	transport2.observer.once('newdataproducer', onObserverNewDataProducer);

	dataProducer2 = await transport2.produceData(
		{
			sctpStreamParameters :
			{
				streamId       : 777,
				maxRetransmits : 3
			},
			label    : 'foo',
			protocol : 'bar',
			appData  : { foo: 1, bar: '2' }
		});

	expect(onObserverNewDataProducer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataProducer).toHaveBeenCalledWith(dataProducer2);
	expect(dataProducer2.id).toBeType('string');
	expect(dataProducer2.closed).toBe(false);
	expect(dataProducer2.type).toBe('sctp');
	expect(dataProducer2.sctpStreamParameters).toBeType('object');
	expect(dataProducer2.sctpStreamParameters.streamId).toBe(777);
	expect(dataProducer2.sctpStreamParameters.ordered).toBe(false);
	expect(dataProducer2.sctpStreamParameters.maxPacketLifeTime).toBeUndefined();
	expect(dataProducer2.sctpStreamParameters.maxRetransmits).toBe(3);
	expect(dataProducer2.label).toBe('foo');
	expect(dataProducer2.protocol).toBe('bar');
	expect(dataProducer2.appData).toEqual({ foo: 1, bar: '2' });

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds : { [dataProducer2.id]: [] },
				mapDataConsumerIdDataProducerId  : {}
			});

	await expect(transport2.dump())
		.resolves
		.toMatchObject(
			{
				id              : transport2.id,
				dataProducerIds : [ dataProducer2.id ],
				dataConsumerIds : []
			});
}, 2000);

test('transport1.produceData() with wrong arguments rejects with TypeError', async () =>
{
	await expect(transport1.produceData({}))
		.rejects
		.toThrow(TypeError);

	// Missing or empty sctpStreamParameters.streamId.
	await expect(transport1.produceData(
		{
			sctpStreamParameters : { foo: 'foo' }
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('transport.produceData() with already used streamId rejects with Error', async () =>
{
	await expect(transport1.produceData(
		{
			sctpStreamParameters :
			{
				streamId : 666
			}
		}))
		.rejects
		.toThrow(Error);
}, 2000);

test('transport.produceData() with ordered and maxPacketLifeTime rejects with TypeError', async () =>
{
	await expect(transport1.produceData(
		{
			sctpStreamParameters :
			{
				streamId          : 999,
				ordered           : true,
				maxPacketLifeTime : 4000
			}
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('dataProducer.dump() succeeds', async () =>
{
	let data;

	data = await dataProducer1.dump();

	expect(data.id).toBe(dataProducer1.id);
	expect(data.type).toBe('sctp');
	expect(data.sctpStreamParameters).toBeType('object');
	expect(data.sctpStreamParameters.streamId).toBe(666);
	expect(data.sctpStreamParameters.ordered).toBe(true);
	expect(data.sctpStreamParameters.maxPacketLifeTime).toBeUndefined();
	expect(data.sctpStreamParameters.maxRetransmits).toBeUndefined();
	expect(data.label).toBe('foo');
	expect(data.protocol).toBe('bar');

	data = await dataProducer2.dump();

	expect(data.id).toBe(dataProducer2.id);
	expect(data.type).toBe('sctp');
	expect(data.sctpStreamParameters).toBeType('object');
	expect(data.sctpStreamParameters.streamId).toBe(777);
	expect(data.sctpStreamParameters.ordered).toBe(false);
	expect(data.sctpStreamParameters.maxPacketLifeTime).toBeUndefined();
	expect(data.sctpStreamParameters.maxRetransmits).toBe(3);
	expect(data.label).toBe('foo');
	expect(data.protocol).toBe('bar');
}, 2000);

test('dataProducer.getStats() succeeds', async () =>
{
	await expect(dataProducer1.getStats())
		.resolves
		.toMatchObject(
			[
				{
					type             : 'data-producer',
					label            : dataProducer1.label,
					protocol         : dataProducer1.protocol,
					messagesReceived : 0,
					bytesReceived    : 0
				}
			]);

	await expect(dataProducer2.getStats())
		.resolves
		.toMatchObject(
			[
				{
					type             : 'data-producer',
					label            : dataProducer2.label,
					protocol         : dataProducer2.protocol,
					messagesReceived : 0,
					bytesReceived    : 0
				}
			]);
}, 2000);

test('dataProducer.close() succeeds', async () =>
{
	const onObserverClose = jest.fn();

	dataProducer1.observer.once('close', onObserverClose);
	dataProducer1.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataProducer1.closed).toBe(true);

	await expect(router.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds : {},
				mapDataConsumerIdDataProducerId  : {}
			});

	await expect(transport1.dump())
		.resolves
		.toMatchObject(
			{
				id              : transport1.id,
				dataProducerIds : [],
				dataConsumerIds : []
			});
}, 2000);

test('DataProducer methods reject if closed', async () =>
{
	await expect(dataProducer1.dump())
		.rejects
		.toThrow(Error);

	await expect(dataProducer1.getStats())
		.rejects
		.toThrow(Error);
}, 2000);

test('DataProducer emits "transportclose" if Transport is closed', async () =>
{
	const onObserverClose = jest.fn();

	dataProducer2.observer.once('close', onObserverClose);

	await new Promise((resolve) =>
	{
		dataProducer2.on('transportclose', resolve);
		transport2.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataProducer2.closed).toBe(true);
}, 2000);
