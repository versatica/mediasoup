import * as dgram from 'node:dgram';
// @ts-ignore
import * as sctp from 'sctp';
import * as mediasoup from '../';

// Set node-sctp default PMTU to 1200.
sctp.defaults({ PMTU: 1200 });

let worker: mediasoup.types.Worker;
let router: mediasoup.types.Router;
let transport: mediasoup.types.PlainTransport;
let dataProducer: mediasoup.types.DataProducer;
let dataConsumer: mediasoup.types.DataConsumer;
let udpSocket: dgram.Socket;
let sctpSocket: any;
let sctpSendStreamId: number;
let sctpSendStream: any;

beforeAll(async () =>
{
	worker = await mediasoup.createWorker();
	router = await worker.createRouter();
	transport = await router.createPlainTransport(
		{
			listenIp       : '127.0.0.1', // https://github.com/nodejs/node/issues/14900
			comedia        : true, // So we don't need to call transport.connect().
			enableSctp     : true,
			numSctpStreams : { OS: 256, MIS: 256 }
		});

	// Node UDP socket for SCTP.
	udpSocket = dgram.createSocket({ type: 'udp4' });

	await new Promise<void>((resolve) => udpSocket.bind(0, '127.0.0.1', resolve));

	const remoteUdpIp = transport.tuple.localIp;
	const remoteUdpPort = transport.tuple.localPort;
	const { OS, MIS } = transport.sctpParameters!;

	// Use UDP connected socket if Node >= 12.
	if (typeof udpSocket.connect === 'function')
	{
		await new Promise<void>((resolve, reject) =>
		{
			// @ts-ignore
			udpSocket.connect(remoteUdpPort, remoteUdpIp, (error: Error) =>
			{
				if (error)
				{
					reject(error);

					return;
				}

				sctpSocket = sctp.connect(
					{
						localPort    : 5000, // Required for SCTP over UDP in mediasoup.
						port         : 5000, // Required for SCTP over UDP in mediasoup.
						OS           : OS,
						MIS          : MIS,
						udpTransport : udpSocket
					});

				resolve();
			});
		});
	}
	// Use UDP disconnected socket if Node < 12.
	else
	{
		sctpSocket = sctp.connect(
			{
				localPort    : 5000, // Required for SCTP over UDP in mediasoup.
				port         : 5000, // Required for SCTP over UDP in mediasoup.
				OS           : OS,
				MIS          : MIS,
				udpTransport : udpSocket,
				udpPeer      :
				{
					address : remoteUdpIp,
					port    : remoteUdpPort
				}
			});
	}

	// Wait for the SCTP association to be open.
	await Promise.race(
		[
			new Promise<void>((resolve) => sctpSocket.on('connect', resolve)),
			new Promise<void>((resolve, reject) => (
				setTimeout(() => reject(new Error('SCTP connection timeout')), 3000)
			))
		]);

	// Create an explicit SCTP outgoing stream with id 123 (id 0 is already used
	// by the implicit SCTP outgoing stream built-in the SCTP socket).
	sctpSendStreamId = 123;
	sctpSendStream = sctpSocket.createStream(sctpSendStreamId);

	// Create a DataProducer with the corresponding SCTP stream id.
	dataProducer = await transport.produceData(
		{
			sctpStreamParameters :
			{
				streamId : sctpSendStreamId,
				ordered  : true
			},
			label    : 'node-sctp',
			protocol : 'foo & bar 😀😀😀'
		});

	// Create a DataConsumer to receive messages from the DataProducer over the
	// same transport.
	dataConsumer = await transport.consumeData({ dataProducerId: dataProducer.id });
});

afterAll(() =>
{
	udpSocket.close();
	sctpSocket.end();
	worker.close();
});

test('ordered DataProducer delivers all SCTP messages to the DataConsumer', async () =>
{
	const onStream = jest.fn();
	const numMessages = 200;
	let sentMessageBytes = 0;
	let recvMessageBytes = 0;
	let lastSentMessageId = 0;
	let lastRecvMessageId = 0;

	// It must be zero because it's the first DataConsumer on the transport.
	expect(dataConsumer.sctpStreamParameters?.streamId).toBe(0);

	await new Promise<void>((resolve) =>
	{
		// Send SCTP messages over the sctpSendStream created above.
		const interval = setInterval(() =>
		{
			const id = ++lastSentMessageId;
			const data = Buffer.from(String(id));

			// Set ppid of type WebRTC DataChannel string.
			if (id < numMessages / 2)
			{
				// @ts-ignore
				data.ppid = sctp.PPID.WEBRTC_STRING;
			}
			// Set ppid of type WebRTC DataChannel binary.
			else
			{
				// @ts-ignore
				data.ppid = sctp.PPID.WEBRTC_BINARY;
			}

			sctpSendStream.write(data);
			sentMessageBytes += data.byteLength;

			if (id === numMessages)
			{
				clearInterval(interval);
			}
		}, 10);

		sctpSocket.on('stream', onStream);

		// Handle the generated SCTP incoming stream and SCTP messages receives on it.
		// @ts-ignore
		sctpSocket.on('stream', (stream, streamId) =>
		{
			// It must be zero because it's the first SCTP incoming stream (so first
			// DataConsumer).
			expect(streamId).toBe(0);

			// @ts-ignore
			stream.on('data', (data: Buffer) =>
			{
				recvMessageBytes += data.byteLength;

				const id = Number(data.toString('utf8'));

				if (id === numMessages)
				{
					clearInterval(interval);
					resolve();
				}

				if (id < numMessages / 2)
				{
					// @ts-ignore
					expect(data.ppid).toBe(sctp.PPID.WEBRTC_STRING);
				}
				else
				{
					// @ts-ignore
					expect(data.ppid).toBe(sctp.PPID.WEBRTC_BINARY);
				}

				expect(id).toBe(++lastRecvMessageId);
			});
		});
	});

	expect(onStream).toHaveBeenCalledTimes(1);
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
}, 10000);
