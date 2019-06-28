const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;

expect.extend({ toBeType });

let worker;
let router;
let webRtcTransport;
let plainRtpTransport;
let dataProducer1;
let dataProducer2;

beforeAll(async () =>
{
	worker = await createWorker();
	router = await worker.createRouter();
	webRtcTransport = await router.createWebRtcTransport(
		{
			listenIps  : [ '127.0.0.1' ],
			enableSctp : true
		});
	plainRtpTransport = await router.createPlainRtpTransport(
		{
			listenIp   : '127.0.0.1',
			enableSctp : true
		});
});

afterAll(() => worker.close());

test('webRtcTransport.produceData() succeeds', async () =>
{
	const onObserverNewDataProducer = jest.fn();

	webRtcTransport.observer.once('newdataproducer', onObserverNewDataProducer);

	dataProducer1 = await webRtcTransport.produceData(
		{
			sctpStreamParameters :
			{
				streamId          : 666,
				ordered           : true,
				maxPacketLifeTime : 5000
			},
			label    : 'foo',
			protocol : 'bar',
			appData  : { foo: 1, bar: '2' }
		});

	expect(onObserverNewDataProducer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataProducer).toHaveBeenCalledWith(dataProducer1);
	expect(dataProducer1.id).toBeType('string');
	expect(dataProducer1.closed).toBe(false);
	expect(dataProducer1.sctpStreamParameters).toBeType('object');
	expect(dataProducer1.sctpStreamParameters.streamId).toBe(666);
	expect(dataProducer1.sctpStreamParameters.ordered).toBe(true);
	expect(dataProducer1.sctpStreamParameters.maxPacketLifeTime).toBe(5000);
	expect(dataProducer1.sctpStreamParameters.maxRetransmits).toBe(undefined);
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

	await expect(webRtcTransport.dump())
		.resolves
		.toMatchObject(
			{
				id              : webRtcTransport.id,
				dataProducerIds : [ dataProducer1.id ],
				dataConsumerIds : []
			});
}, 2000);

test('plainRtpTransport.produceData() succeeds', async () =>
{
	const onObserverNewDataProducer = jest.fn();

	plainRtpTransport.observer.once('newdataproducer', onObserverNewDataProducer);

	dataProducer2 = await plainRtpTransport.produceData(
		{
			sctpStreamParameters :
			{
				streamId          : 6666,
				ordered           : true,
				maxPacketLifeTime : 50000
			},
			label    : 'foo',
			protocol : 'bar',
			appData  : { foo: 1, bar: '2' }
		});

	expect(onObserverNewDataProducer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataProducer).toHaveBeenCalledWith(dataProducer2);
	expect(dataProducer2.id).toBeType('string');
	expect(dataProducer2.closed).toBe(false);
	expect(dataProducer2.sctpStreamParameters).toBeType('object');
	expect(dataProducer2.sctpStreamParameters.streamId).toBe(6666);
	expect(dataProducer2.sctpStreamParameters.ordered).toBe(true);
	expect(dataProducer2.sctpStreamParameters.maxPacketLifeTime).toBe(50000);
	expect(dataProducer2.sctpStreamParameters.maxRetransmits).toBe(undefined);
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

	await expect(plainRtpTransport.dump())
		.resolves
		.toMatchObject(
			{
				id              : plainRtpTransport.id,
				dataProducerIds : [ dataProducer2.id ],
				dataConsumerIds : []
			});
}, 2000);

test('webRtcTransport.produceData() with wrong arguments rejects with TypeError', async () =>
{
	await expect(webRtcTransport.produceData({}))
		.rejects
		.toThrow(TypeError);

	// Missing or empty sctpStreamParameters.streamId.
	await expect(webRtcTransport.produceData(
		{
			sctpStreamParameters : { foo: 'foo' }
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('transport.produceData() with already used streamId rejects with Error', async () =>
{
	await expect(webRtcTransport.produceData(
		{
			sctpStreamParameters :
			{
				streamId : 666
			}
		}))
		.rejects
		.toThrow(Error);
}, 2000);

test('dataProducer.dump() succeeds', async () =>
{
	let data;

	data = await dataProducer1.dump();

	expect(data.id).toBe(dataProducer1.id);
	expect(data.sctpStreamParameters).toBeType('object');
	expect(data.sctpStreamParameters.streamId).toBe(666);
	expect(data.sctpStreamParameters.ordered).toBe(true);
	expect(data.sctpStreamParameters.maxPacketLifeTime).toBe(5000);
	expect(data.sctpStreamParameters.maxRetransmits).toBe(undefined);
	expect(data.sctpStreamParameters.reliable).toBe(false);
	expect(data.label).toBe('foo');
	expect(data.protocol).toBe('bar');

	data = await dataProducer2.dump();

	expect(data.id).toBe(dataProducer2.id);
	expect(data.sctpStreamParameters).toBeType('object');
	expect(data.sctpStreamParameters.streamId).toBe(6666);
	expect(data.sctpStreamParameters.ordered).toBe(true);
	expect(data.sctpStreamParameters.maxPacketLifeTime).toBe(50000);
	expect(data.sctpStreamParameters.maxRetransmits).toBe(undefined);
	expect(data.label).toBe('foo');
	expect(data.protocol).toBe('bar');
}, 2000);

test('dataProducer.getStats() succeeds', async () =>
{
	await expect(dataProducer1.getStats())
		.resolves
		.toStrictEqual(
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
		.toStrictEqual(
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

	await expect(webRtcTransport.dump())
		.resolves
		.toMatchObject(
			{
				id              : webRtcTransport.id,
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
		plainRtpTransport.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataProducer2.closed).toBe(true);
}, 2000);
