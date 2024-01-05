import * as mediasoup from '../';
import * as utils from '../utils';

type TestContext =
{
	dataProducerParameters1: mediasoup.types.DataProducerOptions;
	dataProducerParameters2: mediasoup.types.DataProducerOptions;
	worker?: mediasoup.types.Worker;
	router?: mediasoup.types.Router;
	transport1?: mediasoup.types.WebRtcTransport;
	transport2?: mediasoup.types.WebRtcTransport;
};

const ctx: TestContext =
{
	dataProducerParameters1 : utils.deepFreeze(
		{
			sctpStreamParameters :
			{
				streamId : 666
			},
			label    : 'foo',
			protocol : 'bar',
			appData  : { foo: 1, bar: '2' }
		}
	),
	dataProducerParameters2 : utils.deepFreeze(
		{
			sctpStreamParameters :
			{
				streamId       : 777,
				maxRetransmits : 3
			},
			label    : 'foo',
			protocol : 'bar',
			paused   : true,
			appData  : { foo: 1, bar: '2' }
		}
	)
};

beforeEach(async () =>
{
	ctx.worker = await mediasoup.createWorker();
	ctx.router = await ctx.worker.createRouter();
	ctx.transport1 = await ctx.router.createWebRtcTransport(
		{
			listenIps  : [ '127.0.0.1' ],
			enableSctp : true
		});
	ctx.transport2 = await ctx.router.createWebRtcTransport(
		{
			listenIps  : [ '127.0.0.1' ],
			enableSctp : true
		});
});

afterEach(() =>
{
	ctx.worker?.close();
});

test('transport1.produceData() succeeds', async () =>
{
	const onObserverNewDataProducer = jest.fn();

	ctx.transport1!.observer.once('newdataproducer', onObserverNewDataProducer);

	const dataProducer1 =
		await ctx.transport1!.produceData(ctx.dataProducerParameters1);

	expect(onObserverNewDataProducer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataProducer).toHaveBeenCalledWith(dataProducer1);
	expect(typeof dataProducer1.id).toBe('string');
	expect(dataProducer1.closed).toBe(false);
	expect(dataProducer1.type).toBe('sctp');
	expect(typeof dataProducer1.sctpStreamParameters).toBe('object');
	expect(dataProducer1.sctpStreamParameters?.streamId).toBe(666);
	expect(dataProducer1.sctpStreamParameters?.ordered).toBe(true);
	expect(dataProducer1.sctpStreamParameters?.maxPacketLifeTime).toBeUndefined();
	expect(dataProducer1.sctpStreamParameters?.maxRetransmits).toBeUndefined();
	expect(dataProducer1.label).toBe('foo');
	expect(dataProducer1.protocol).toBe('bar');
	expect(dataProducer1.paused).toBe(false);
	expect(dataProducer1.appData).toEqual({ foo: 1, bar: '2' });

	const dump = await ctx.router!.dump();

	expect(dump.mapDataProducerIdDataConsumerIds)
		.toEqual(expect.arrayContaining([
			{ key: dataProducer1.id, values: [ ] }
		]));

	expect(dump.mapDataConsumerIdDataProducerId.length).toBe(0);

	await expect(ctx.transport1!.dump())
		.resolves
		.toMatchObject(
			{
				id              : ctx.transport1!.id,
				dataProducerIds : [ dataProducer1.id ],
				dataConsumerIds : []
			});
}, 2000);

test('transport2.produceData() succeeds', async () =>
{
	const onObserverNewDataProducer = jest.fn();

	ctx.transport2!.observer.once('newdataproducer', onObserverNewDataProducer);

	const dataProducer2 =
		await ctx.transport2!.produceData(ctx.dataProducerParameters2);

	expect(onObserverNewDataProducer).toHaveBeenCalledTimes(1);
	expect(onObserverNewDataProducer).toHaveBeenCalledWith(dataProducer2);
	expect(typeof dataProducer2.id).toBe('string');
	expect(dataProducer2.closed).toBe(false);
	expect(dataProducer2.type).toBe('sctp');
	expect(typeof dataProducer2.sctpStreamParameters).toBe('object');
	expect(dataProducer2.sctpStreamParameters?.streamId).toBe(777);
	expect(dataProducer2.sctpStreamParameters?.ordered).toBe(false);
	expect(dataProducer2.sctpStreamParameters?.maxPacketLifeTime).toBeUndefined();
	expect(dataProducer2.sctpStreamParameters?.maxRetransmits).toBe(3);
	expect(dataProducer2.label).toBe('foo');
	expect(dataProducer2.protocol).toBe('bar');
	expect(dataProducer2.paused).toBe(true);
	expect(dataProducer2.appData).toEqual({ foo: 1, bar: '2' });

	const dump = await ctx.router!.dump();

	expect(dump.mapDataProducerIdDataConsumerIds)
		.toEqual(expect.arrayContaining([
			{ key: dataProducer2.id, values: [ ] }
		]));

	expect(dump.mapDataConsumerIdDataProducerId.length).toBe(0);

	await expect(ctx.transport2!.dump())
		.resolves
		.toMatchObject(
			{
				id              : ctx.transport2!.id,
				dataProducerIds : [ dataProducer2.id ],
				dataConsumerIds : []
			});
}, 2000);

test('transport1.produceData() with wrong arguments rejects with TypeError', async () =>
{
	await expect(ctx.transport1!.produceData({}))
		.rejects
		.toThrow(TypeError);

	// Missing or empty sctpStreamParameters.streamId.
	await expect(ctx.transport1!.produceData(
		{
			// @ts-ignore
			sctpStreamParameters : { foo: 'foo' }
		}))
		.rejects
		.toThrow(TypeError);
}, 2000);

test('transport.produceData() with already used streamId rejects with Error', async () =>
{
	await ctx.transport1!.produceData(ctx.dataProducerParameters1);

	await expect(ctx.transport1!.produceData(
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
	await expect(ctx.transport1!.produceData(
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
	const dataProducer1 =
		await ctx.transport1!.produceData(ctx.dataProducerParameters1);

	const dump1 = await dataProducer1.dump();

	expect(dump1.id).toBe(dataProducer1.id);
	expect(dump1.type).toBe('sctp');
	expect(typeof dump1.sctpStreamParameters).toBe('object');
	expect(dump1.sctpStreamParameters!.streamId).toBe(666);
	expect(dump1.sctpStreamParameters!.ordered).toBe(true);
	expect(dump1.sctpStreamParameters!.maxPacketLifeTime).toBeUndefined();
	expect(dump1.sctpStreamParameters!.maxRetransmits).toBeUndefined();
	expect(dump1.label).toBe('foo');
	expect(dump1.protocol).toBe('bar');
	expect(dump1.paused).toBe(false);

	const dataProducer2 =
		await ctx.transport2!.produceData(ctx.dataProducerParameters2);

	const dump2 = await dataProducer2.dump();

	expect(dump2.id).toBe(dataProducer2.id);
	expect(dump2.type).toBe('sctp');
	expect(typeof dump2.sctpStreamParameters).toBe('object');
	expect(dump2.sctpStreamParameters!.streamId).toBe(777);
	expect(dump2.sctpStreamParameters!.ordered).toBe(false);
	expect(dump2.sctpStreamParameters!.maxPacketLifeTime).toBeUndefined();
	expect(dump2.sctpStreamParameters!.maxRetransmits).toBe(3);
	expect(dump2.label).toBe('foo');
	expect(dump2.protocol).toBe('bar');
	expect(dump2.paused).toBe(true);
}, 2000);

test('dataProducer.getStats() succeeds', async () =>
{
	const dataProducer1 =
		await ctx.transport1!.produceData(ctx.dataProducerParameters1);

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

	const dataProducer2 =
		await ctx.transport2!.produceData(ctx.dataProducerParameters2);

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

test('dataProducer.pause() and resume() succeed', async () =>
{
	const dataProducer1 =
		await ctx.transport1!.produceData(ctx.dataProducerParameters1);

	const onObserverPause = jest.fn();
	const onObserverResume = jest.fn();

	dataProducer1.observer.on('pause', onObserverPause);
	dataProducer1.observer.on('resume', onObserverResume);

	await dataProducer1.pause();

	expect(dataProducer1.paused).toBe(true);

	const dump1 = await dataProducer1.dump();

	expect(dump1.paused).toBe(true);

	await dataProducer1.resume();

	expect(dataProducer1.paused).toBe(false);

	const dump2 = await dataProducer1.dump();

	expect(dump2.paused).toBe(false);

	// Even if we don't await for pause()/resume() completion, the observer must
	// fire 'pause' and 'resume' events if state was the opposite.
	dataProducer1.pause();
	dataProducer1.resume();
	dataProducer1.pause();
	dataProducer1.pause();
	dataProducer1.pause();
	await dataProducer1.resume();

	expect(onObserverPause).toHaveBeenCalledTimes(3);
	expect(onObserverResume).toHaveBeenCalledTimes(3);
}, 2000);

test('producer.pause() and resume() emit events', async () =>
{
	const dataProducer1 =
		await ctx.transport1!.produceData(ctx.dataProducerParameters1);

	const promises = [];
	const events: string[] = [];
	
	dataProducer1.observer.once('resume', () =>
	{
		events.push('resume');
	});

	dataProducer1.observer.once('pause', () =>
	{
		events.push('pause');
	});

	promises.push(dataProducer1.pause());
	promises.push(dataProducer1.resume());

	await Promise.all(promises);
	
	expect(events).toEqual([ 'pause', 'resume' ]);
	expect(dataProducer1.paused).toBe(false);
}, 2000);

test('dataProducer.close() succeeds', async () =>
{
	const dataProducer1 =
		await ctx.transport1!.produceData(ctx.dataProducerParameters1);

	const onObserverClose = jest.fn();

	dataProducer1.observer.once('close', onObserverClose);
	dataProducer1.close();

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataProducer1.closed).toBe(true);

	await expect(ctx.router!.dump())
		.resolves
		.toMatchObject(
			{
				mapDataProducerIdDataConsumerIds : {},
				mapDataConsumerIdDataProducerId  : {}
			});

	await expect(ctx.transport1!.dump())
		.resolves
		.toMatchObject(
			{
				id              : ctx.transport1!.id,
				dataProducerIds : [],
				dataConsumerIds : []
			});
}, 2000);

test('DataProducer methods reject if closed', async () =>
{
	const dataProducer1 =
		await ctx.transport1!.produceData(ctx.dataProducerParameters1);

	dataProducer1.close();

	await expect(dataProducer1.dump())
		.rejects
		.toThrow(Error);

	await expect(dataProducer1.getStats())
		.rejects
		.toThrow(Error);
}, 2000);

test('DataProducer emits "transportclose" if Transport is closed', async () =>
{
	const dataProducer2 =
		await ctx.transport2!.produceData(ctx.dataProducerParameters2);

	const onObserverClose = jest.fn();

	dataProducer2.observer.once('close', onObserverClose);

	await new Promise<void>((resolve) =>
	{
		dataProducer2.on('transportclose', resolve);
		ctx.transport2!.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(dataProducer2.closed).toBe(true);
}, 2000);
