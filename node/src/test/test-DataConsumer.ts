import * as mediasoup from '../';

let worker: mediasoup.types.Worker;
let router: mediasoup.types.Router;
let transport1: mediasoup.types.WebRtcTransport;
let transport2: mediasoup.types.PlainTransport;
let transport3: mediasoup.types.DirectTransport;
let dataProducer: mediasoup.types.DataProducer;
let dataConsumer1: mediasoup.types.DataConsumer;
let dataConsumer2: mediasoup.types.DataConsumer;

const dataProducerParameters: mediasoup.types.DataProducerOptions =
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
	worker = await mediasoup.createWorker();
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
			// Valid values are 0...65535 so others and duplicated ones will be
			// discarded.
			subchannels       : [ 0, 1, 1, 1, 2, 65535, 65536, 65537, 100 ],
			appData           : { baz: 'LOL' }
		});

	expect(onObserverNewDataConsumer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataConsumer).toHaveBeenCalledWith(dataConsumer1);
	expect(typeof dataConsumer1.id).toBe('string');
	expect(dataConsumer1.dataProducerId).toBe(dataProducer.id);
	expect(dataConsumer1.closed).toBe(false);
	expect(dataConsumer1.type).toBe('sctp');
	expect(typeof dataConsumer1.sctpStreamParameters).toBe('object');
	expect(typeof dataConsumer1.sctpStreamParameters?.streamId).toBe('number');
	expect(dataConsumer1.sctpStreamParameters?.ordered).toBe(false);
	expect(dataConsumer1.sctpStreamParameters?.maxPacketLifeTime).toBe(4000);
	expect(dataConsumer1.sctpStreamParameters?.maxRetransmits).toBeUndefined();
	expect(dataConsumer1.label).toBe('foo');
	expect(dataConsumer1.protocol).toBe('bar');
	expect(dataConsumer1.paused).toBe(false);
	expect(dataConsumer1.subchannels.sort((a, b) => a - b))
		.toEqual([ 0, 1, 2, 100, 65535 ]);
	expect(dataConsumer1.appData).toEqual({ baz: 'LOL' });

	const dump = await router.dump();

	expect(dump.mapDataProducerIdDataConsumerIds)
		.toEqual(expect.arrayContaining([
			{ key: dataProducer.id, values: [ dataConsumer1.id ] }
		]));

	expect(dump.mapDataConsumerIdDataProducerId)
		.toEqual(expect.arrayContaining([
			{ key: dataConsumer1.id, value: dataProducer.id }
		]));

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
	expect(typeof data.sctpStreamParameters).toBe('object');
	expect(data.sctpStreamParameters!.streamId)
		.toBe(dataConsumer1.sctpStreamParameters?.streamId);
	expect(data.sctpStreamParameters!.ordered).toBe(false);
	expect(data.sctpStreamParameters!.maxPacketLifeTime).toBe(4000);
	expect(data.sctpStreamParameters!.maxRetransmits).toBeUndefined();
	expect(data.label).toBe('foo');
	expect(data.protocol).toBe('bar');
	expect(data.paused).toBe(false);
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

test('dataConsumer.setSubchannels() succeeds', async () =>
{
	await dataConsumer1.setSubchannels([ 999, 999, 998, 65536 ]);

	expect(dataConsumer1.subchannels.sort((a, b) => a - b))
		.toEqual([ 0, 998, 999 ]);
}, 2000);

test('dataConsumer.addSubchannel() and .removeSubchannel() succeed', async () =>
{
	await dataConsumer1.setSubchannels([ ]);
	expect(dataConsumer1.subchannels).toEqual([ ]);

	await dataConsumer1.addSubchannel(5);
	expect(dataConsumer1.subchannels.sort((a, b) => a - b)).toEqual([ 5 ]);

	await dataConsumer1.addSubchannel(10);
	expect(dataConsumer1.subchannels.sort((a, b) => a - b)).toEqual([ 5, 10 ]);

	await dataConsumer1.addSubchannel(5);
	expect(dataConsumer1.subchannels.sort((a, b) => a - b)).toEqual([ 5, 10 ]);

	await dataConsumer1.removeSubchannel(666);
	expect(dataConsumer1.subchannels.sort((a, b) => a - b)).toEqual([ 5, 10 ]);

	await dataConsumer1.removeSubchannel(5);
	expect(dataConsumer1.subchannels.sort((a, b) => a - b)).toEqual([ 10 ]);

	await dataConsumer1.setSubchannels([ ]);
	expect(dataConsumer1.subchannels).toEqual([ ]);
}, 2000);

test('transport.consumeData() on a DirectTransport succeeds', async () =>
{
	const onObserverNewDataConsumer = jest.fn();

	transport3.observer.once('newdataconsumer', onObserverNewDataConsumer);

	dataConsumer2 = await transport3.consumeData(
		{
			dataProducerId : dataProducer.id,
			paused         : true,
			appData        : { hehe: 'HEHE' }
		});

	expect(onObserverNewDataConsumer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataConsumer).toHaveBeenCalledWith(dataConsumer2);
	expect(typeof dataConsumer2.id).toBe('string');
	expect(dataConsumer2.dataProducerId).toBe(dataProducer.id);
	expect(dataConsumer2.closed).toBe(false);
	expect(dataConsumer2.type).toBe('direct');
	expect(dataConsumer2.sctpStreamParameters).toBeUndefined();
	expect(dataConsumer2.label).toBe('foo');
	expect(dataConsumer2.protocol).toBe('bar');
	expect(dataConsumer2.paused).toBe(true);
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
	expect(data.paused).toBe(true);
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

test('dataConsumer.pause() and resume() succeed', async () =>
{
	const onObserverPause = jest.fn();
	const onObserverResume = jest.fn();

	dataConsumer1.observer.on('pause', onObserverPause);
	dataConsumer1.observer.on('resume', onObserverResume);

	let data;

	await dataConsumer1.pause();

	expect(dataConsumer1.paused).toBe(true);

	data = await dataConsumer1.dump();

	expect(data.paused).toBe(true);

	await dataConsumer1.resume();

	expect(dataConsumer1.paused).toBe(false);

	data = await dataConsumer1.dump();

	expect(data.paused).toBe(false);

	// Even if we don't await for pause()/resume() completion, the observer must
	// fire 'pause' and 'resume' events if state was the opposite.
	dataConsumer1.pause();
	dataConsumer1.resume();
	dataConsumer1.pause();
	dataConsumer1.pause();
	dataConsumer1.pause();
	await dataConsumer1.resume();

	expect(onObserverPause).toHaveBeenCalledTimes(3);
	expect(onObserverResume).toHaveBeenCalledTimes(3);
}, 2000);

test('dataProducer.pause() and resume() emit events', async () =>
{
	const promises = [];
	const events: string[] = [];
	
	dataConsumer1.observer.once('resume', () => 
	{
		events.push('resume');
	});

	dataConsumer1.observer.once('pause', () => 
	{
		events.push('pause');
	});

	promises.push(dataProducer.pause());
	promises.push(dataProducer.resume());

	await Promise.all(promises);

	// Must also wait a bit for the corresponding events in the data consumer.
	await new Promise((resolve) => setTimeout(resolve, 100));
	
	expect(events).toEqual([ 'pause', 'resume' ]);
	expect(dataConsumer1.paused).toBe(false);
}, 2000);

test('dataConsumer.close() succeeds', async () =>
{
	const onObserverClose = jest.fn();

	dataConsumer1.observer.once('close', onObserverClose);
	dataConsumer1.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataConsumer1.closed).toBe(true);

	const dump = await router.dump();

	expect(dump.mapDataProducerIdDataConsumerIds)
		.toEqual(expect.arrayContaining([
			{ key: dataProducer.id, values: [ dataConsumer2.id ] }
		]));

	expect(dump.mapDataConsumerIdDataProducerId)
		.toEqual(expect.arrayContaining([
			{ key: dataConsumer2.id, value: dataProducer.id }
		]));

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

	await new Promise<void>((resolve) =>
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

	await new Promise<void>((resolve) =>
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
