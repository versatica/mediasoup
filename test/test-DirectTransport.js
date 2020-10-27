const { toBeType } = require('jest-tobetype');
const mediasoup = require('../');
const { createWorker } = mediasoup;

// "standard" parameters for an audio-only
// test of producers and consumers
const mediaCodecs = [
	{
		kind       : 'video',
		mimeType   : 'video/VP8',
		clockRate  : 90000,
		parameters : { 'x-google-start-bitrate': 1000 }
	}
];

const produceOptions = {
	'rtpParameters' : {
		'codecs' : [
			{ 
				'mimeType'     : 'video/VP8', 
				'payloadType'  : 101,
				'clockRate'    : 90000,
				'parameters'   : {},
				'rtcpFeedback' : [
					{ 'type': 'goog-remb', 'parameter': '' },
					{ 'type': 'transport-cc', 'parameter': '' },
					{ 'type': 'ccm', 'parameter': 'fir' },
					{ 'type': 'nack', 'parameter': '' },
					{ 'type': 'nack', 'parameter': 'pli' }
				]
			}
		],
		'headerExtensions' : [ 
			{ 'uri': 'urn:ietf:params:rtp-hdrext:sdes:mid', 'id': 4, 'encrypt': false, 'parameters': {} },
			{ 'uri': 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id', 'id': 5, 'encrypt': false, 'parameters': {} },
			{ 'uri': 'urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id', 'id': 6, 'encrypt': false, 'parameters': {} },
			{ 'uri': 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time', 'id': 2, 'encrypt': false, 'parameters': {} },
			{ 'uri': 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01', 'id': 3, 'encrypt': false, 'parameters': {} },
			{ 'uri': 'urn:3gpp:video-orientation', 'id': 13, 'encrypt': false, 'parameters': {} },
			{ 'uri': 'urn:ietf:params:rtp-hdrext:toffset', 'id': 14, 'encrypt': false, 'parameters': {} }
		],
		'encodings' : [
			{ 'active': true, 'scalabilityMode': 'S1T3', 'maxBitrate': 100000, 'rid': 'r0', 'dtx': false, 'ssrc': 391513779 },
			{ 'active': true, 'scalabilityMode': 'S1T3', 'maxBitrate': 300000, 'rid': 'r1', 'dtx': false },
			{ 'active': true, 'scalabilityMode': 'S1T3', 'maxBitrate': 900000, 'rid': 'r2', 'dtx': false }
		],
		'rtcp' : { 'cname': '75946337', 'reducedSize': true },
		'mid'  : '0'
	},
	'kind' : 'video'
};

const rtpCapabilities = {
	'codecs' : [
		{
			'kind'         : 'video', 
			'mimeType'     : 'video/VP8',
			'clockRate'    : 90000,
			'rtcpFeedback' : [
				{ 'type': 'nack', 'parameter': '' },
				{ 'type': 'nack', 'parameter': 'pli' },
				{ 'type': 'ccm', 'parameter': 'fir' },
				{ 'type': 'goog-remb', 'parameter': '' },
				{ 'type': 'transport-cc', 'parameter': '' }
			], 
			'parameters'           : { 'x-google-start-bitrate': 1000 },
			'preferredPayloadType' : 101
		}
	], 
	'headerExtensions' : [
		{ 'kind': 'video', 'uri': 'urn:ietf:params:rtp-hdrext:sdes:mid', 'preferredId': 1, 'preferredEncrypt': false, 'direction': 'sendrecv' },
		{ 'kind': 'video', 'uri': 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id', 'preferredId': 2, 'preferredEncrypt': false, 'direction': 'recvonly' },
		{ 'kind': 'video', 'uri': 'urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id', 'preferredId': 3, 'preferredEncrypt': false, 'direction': 'recvonly' },
		{ 'kind': 'video', 'uri': 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time', 'preferredId': 4, 'preferredEncrypt': false, 'direction': 'sendrecv' },
		{ 'kind': 'video', 'uri': 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01', 'preferredId': 5, 'preferredEncrypt': false, 'direction': 'sendrecv' },
		{ 'kind': 'video', 'uri': 'http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07', 'preferredId': 6, 'preferredEncrypt': false, 'direction': 'sendrecv' },
		{ 'kind': 'video', 'uri': 'urn:ietf:params:rtp-hdrext:framemarking', 'preferredId': 7, 'preferredEncrypt': false, 'direction': 'sendrecv' },
		{ 'kind': 'video', 'uri': 'urn:3gpp:video-orientation', 'preferredId': 11, 'preferredEncrypt': false, 'direction': 'sendrecv' },
		{ 'kind': 'video', 'uri': 'urn:ietf:params:rtp-hdrext:toffset', 'preferredId': 12, 'preferredEncrypt': false, 'direction': 'sendrecv' }
	]
};

expect.extend({ toBeType });

let worker;
let router;
let transport;

beforeAll(async () =>
{
	worker = await createWorker();
	router = await worker.createRouter({ mediaCodecs });
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
	expect(transport1.id).toBeType('string');
	expect(transport1.closed).toBe(false);
	expect(transport1.appData).toEqual({ foo: 'bar' });

	const data1 = await transport1.dump();

	expect(data1.id).toBe(transport1.id);
	expect(data1.direct).toBe(true);
	expect(data1.producerIds).toEqual([]);
	expect(data1.consumerIds).toEqual([]);
	expect(data1.dataProducerIds).toEqual([]);
	expect(data1.dataConsumerIds).toEqual([]);
	expect(data1.recvRtpHeaderExtensions).toBeType('object');
	expect(data1.rtpListener).toBeType('object');

	transport1.close();
	expect(transport1.closed).toBe(true);

	await expect(router.createDirectTransport())
		.resolves
		.toBeType('object');
}, 2000);

test('transport.produce().send() is heard by transport.consume().on(\'rtp\')', async () => 
{
	const producer = await transport.produce(produceOptions);

	producer.enableTraceEvent([ 'rtp' ]);

	const consumer = await transport.consume({
		producerId : producer.id,
		rtpCapabilities
	});

	consumer.enableTraceEvent([ 'rtp' ]);

	// Create a new packet from all-zero data
	const buf = Buffer.from([ 144, 101, 0, 1, 181, 54,
		88, 7, 23, 86, 6, 179, 190, 222, 0, 4, 16, 48,
		0, 0, 0, 0, 0, 0, 0, 66, 246, 133, 31, 81, 0, 17,
		144, 224, 128, 1, 1, 32, 144, 63, 1, 157, 1, 42,
		0, 5, 208, 2, 57, 107, 0, 39, 28, 36, 12, 44, 44,
		68, 204, 36, 65, 168, 13, 219, 39, 211, 170, 77,
		174, 31, 162, 79, 12, 206, 22, 213, 175, 12, 58,
		141, 71, 187, 1, 183, 89, 127, 240, 134, 204, 218,
		27, 251, 142, 15, 181, 20, 147, 249, 1, 185, 140,
		252, 25, 159, 131, 51, 240, 102, 126, 12, 207, 193,
		153, 248, 51, 63, 6, 103, 224, 204, 252, 25, 159,
		131, 51, 240, 102, 126, 12, 207, 193, 153, 248, 51,
		63, 6, 103, 224, 204, 252, 25, 159, 131, 51, 240,
		102, 126, 12, 207, 193, 153, 248, 51, 63, 6, 103,
		224, 204, 252, 25, 159, 131, 51, 240, 102, 126, 12,
		207, 193, 153, 248, 51, 63, 6, 103, 224, 204, 252,
		25, 159, 131, 51, 240, 102, 126, 12, 207, 193, 153,
		248, 51, 63, 6, 103, 224, 204, 252, 25, 159, 131,
		51, 240, 102, 126, 12, 207, 193, 153, 248, 51, 63,
		6, 103, 224, 204, 252, 25, 159, 131, 51, 240, 102,
		126, 8, 132, 137, 243, 67, 79, 176, 201, 146, 44,
		187, 200, 247, 121, 30, 239, 35, 221, 228, 123, 188,
		143, 119, 145, 238, 242, 61, 222, 71, 187, 200, 247,
		121, 30, 239, 35, 221, 228, 123, 188, 143, 103, 168,
		218, 11, 16, 192, 167, 40, 6, 212, 14, 185, 89, 44,
		215, 187, 200, 247, 121, 30, 239, 35, 221, 228, 123,
		188, 143, 119, 145, 238, 242, 61, 222, 71, 187, 200,
		247, 121, 30, 239, 35, 221, 228, 123, 61, 66, 146,
		253, 104, 188, 169, 149, 17, 105, 120, 69, 160, 192,
		58, 89, 61, 126, 48, 110, 167, 141, 123, 188, 143,
		119, 145, 238, 242, 61, 222, 71, 187, 200, 247, 121,
		30, 239, 35, 221, 228, 123, 188, 143, 119, 145, 238,
		242, 61, 167, 252, 226, 127, 109, 222, 60, 147, 163,
		131, 252, 175, 109, 50, 234, 241, 13, 224, 217, 47,
		76, 68, 217, 214, 102, 251, 198, 126, 12, 207, 193, 
		153, 248, 51, 63, 6, 103, 224, 204, 252, 25, 159, 131,
		51, 240, 102, 126, 12, 207, 193, 153, 235, 4, 81, 36,
		171, 225, 241, 141, 183, 59, 153, 7, 47, 172, 49, 80,
		205, 231, 172, 187, 253, 127, 137, 117, 118, 79, 162,
		124, 175, 73, 157, 121, 52, 41, 35, 25, 248, 51, 63,
		6, 103, 224, 204, 252, 25, 159, 131, 51, 240, 102, 
		126, 12, 207, 193, 153, 248, 51, 63, 4, 84, 210, 90, 49,
		204, 29, 189, 87, 178, 31, 127, 89, 234, 222, 219, 250, 
		30, 157, 56, 200, 168, 207, 48, 151, 242, 185, 216,
		233, 126, 130, 76, 120, 198, 126, 12, 207, 193, 153,
		248, 51, 63, 6, 103, 224, 204, 252, 25, 159, 131, 51,
		240, 102, 126, 12, 202, 57, 87, 110, 30, 126, 183, 7,
		158, 127, 97, 165, 58, 176, 233, 152, 86, 36, 86, 6,
		30, 140, 21, 41, 145, 45, 51, 0, 194, 12, 224, 83, 24,
		230, 51, 240, 102, 126, 12, 207, 193, 153, 248, 51,
		63, 6, 103, 224, 204, 252, 25, 159, 131, 51, 214, 218,
		170, 196, 29, 241, 157, 23, 225, 48, 137, 168, 94,
		85, 88, 45, 215, 222, 153, 140, 30, 147, 83, 115, 239,
		209, 212, 207, 167, 159, 83, 36, 254, 64, 110, 99,
		63, 6, 103, 224, 204, 252, 25, 159, 131, 51, 240, 102,
		126, 12, 207, 193, 153, 89, 74, 241, 211, 244, 190,
		44, 107, 98, 112, 202, 168, 197, 130, 231, 10, 157, 68,
		59, 15, 191, 100, 86, 244, 253, 227, 11, 33, 254,
		193, 252, 195, 2, 42, 145, 245, 0, 49, 159, 131, 51, 
		240, 102, 126, 12, 207, 193, 153, 248, 51, 63, 6, 103,
		224, 204, 252, 25, 159, 117, 58, 154, 246, 239, 192, 
		231, 70, 47, 30, 221, 233, 199, 99, 8, 176, 159, 39, 153,
		87, 149, 123, 108, 163, 179, 148, 65, 168, 59, 9, 41,
		13, 156, 98, 98, 64, 215, 54, 114, 163, 121, 198, 72,
		133, 144, 254, 64, 110, 99, 63, 6, 103, 224, 204, 252,
		25, 159, 131, 51, 240, 102, 126, 12, 207, 193, 145,
		202, 123, 190, 25, 228, 151, 218, 199, 154, 29, 8, 178,
		21, 161, 97, 216, 172, 120, 33, 8, 225, 173, 110, 98,
		57, 178, 29, 22, 31, 90, 175, 169, 103, 253, 3, 16,
		195, 248, 27, 13, 158, 132, 93, 165, 22, 90, 119, 80, 91,
		106, 150, 22, 183, 49, 159, 131, 51, 240, 102, 126, 12,
		207, 193, 153, 248, 51, 63, 6, 103, 224, 204, 252, 18,
		15, 73, 217, 92, 161, 135, 17, 111, 203, 237, 110, 112,
		56, 33, 205, 95, 247, 96, 147, 43, 25, 149, 4, 86, 77,
		133, 95, 80, 95, 61, 76, 43, 219, 8, 96, 164, 87, 200,
		29, 118, 8, 57, 185, 249, 60, 37, 241, 93, 248, 129, 138,
		72, 203, 47, 171, 51, 151, 112, 243, 172, 209, 10, 146,
		140, 244, 27, 201, 33, 173, 52, 24, 239, 35, 148, 133,
		200, 223, 187, 200, 247, 121, 30, 239, 35, 221, 228,
		123, 188, 143, 119, 145, 238, 242, 61, 222, 71, 187, 66,
		165, 232, 148, 4, 56, 161, 26, 185, 227, 234, 247, 25, 
		64, 79, 17, 7, 210, 236, 149, 160, 104, 66, 91, 153, 195,
		252, 66, 203, 48, 111, 54, 120, 17, 153, 128, 120, 130, 
		202, 168, 159, 6, 211, 199, 31, 185, 200, 120, 232, 248,
		96, 16, 0, 191, 108, 237, 118, 160, 90, 220, 250, 109,
		141, 109, 69, 36, 254, 64, 110, 99, 63, 6, 103, 224, 204,
		252, 25, 159, 131, 51, 240, 72, 56, 135, 185, 125, 135,
		238, 75, 102, 114, 80, 56, 22, 84, 80, 46, 188, 81, 178,
		139, 219, 97, 245, 133, 135, 95, 85, 101, 85, 44, 186,
		134, 107, 45, 167, 129, 242, 240, 37, 91, 220, 92, 118,
		225, 195, 149, 5, 133, 10, 210, 75, 207, 88, 69, 86,
		126, 98, 24, 178, 91, 163, 252, 188, 57, 72, 222, 177, 112,
		191, 124, 56, 48, 28, 192, 183, 145, 238, 242, 61, 222,
		71, 187, 200, 247, 121, 30, 239, 35, 221, 228, 123, 188,
		132, 12, 187, 237, 200, 40, 34, 243, 103, 80, 44, 75, 
		121, 171, 156, 137, 223, 119, 34, 51, 4, 229, 43, 190, 193,
		236, 86, 235, 179, 3 ]);
	
	await expect(new Promise((resolve) => 
	{
		consumer.on('rtp', (rtpPacket) => 
		{
			resolve(rtpPacket);
		});
		producer.send(buf);
	})).resolves.toBeType('object');
});

test('router.createDirectTransport() with wrong arguments rejects with TypeError', async () =>
{
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

	expect(data).toBeType('array');
	expect(data.length).toBe(1);
	expect(data[0].type).toBe('direct-transport');
	expect(data[0].transportId).toBeType('string');
	expect(data[0].timestamp).toBeType('number');
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
	expect(data[0].recvBitrate).toBe(0);
	expect(data[0].sendBitrate).toBe(0);
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

	await new Promise((resolve) =>
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
				clearInterval(interval);
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
				expect(ppid).toBe(51); // PPID of WebRTC DataChannel string.
			else
				expect(ppid).toBe(53); // PPID of WebRTC DataChannel binary.

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

	await new Promise((resolve) =>
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

	await new Promise((resolve) =>
	{
		transport.on('routerclose', resolve);
		worker.close();
	});

	expect(onObserverClose).toHaveBeenCalledTimes(1);
	expect(transport.closed).toBe(true);
}, 2000);
