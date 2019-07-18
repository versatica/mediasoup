const { toBeType } = require('jest-tobetype');
const dgram = require('dgram');
const sctp = require('sctp');
const mediasoup = require('../');
const { createWorker } = mediasoup;

expect.extend({ toBeType });

// Set node-sctp default PMTU to 1200.
sctp.defaults({ PMTU: 1200 });

let worker;
let router;
let transport;
let dataProducer;
let dataConsumer;
let udpSocket;
let sctpSocket;
let sendStreamId;
let sctpSendStream;

beforeAll(async () =>
{
	worker = await createWorker();
	router = await worker.createRouter();
	transport = await router.createPlainRtpTransport(
		{
			listenIp       : '127.0.0.1',
			enableSctp     : true,
			numSctpStreams : { OS: 256, MIS: 256 }
		});

	// Node UDP socket for SCTP.
	udpSocket = dgram.createSocket({ type: 'udp4' });

	await new Promise((resolve) => udpSocket.bind(0, '127.0.0.1', resolve));

	const localUdpPort = udpSocket.address().port;

	// Connect the mediasoup PlainRtpTransport to the UDP socket port.
	await transport.connect({ ip: '127.0.0.1', port: localUdpPort });

	const remoteUdpIp = transport.tuple.localIp;
	const remoteUdpPort = transport.tuple.localPort;
	const { OS, MIS } = transport.sctpParameters;

	// Use UDP connected socket if Node >= 12.
	if (typeof udpSocket.connect === 'function')
	{
		await new Promise((resolve, reject) =>
		{
			udpSocket.connect(remoteUdpPort, remoteUdpIp, (error) =>
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

	// Create a explicit SCTP outgoing stream with id 123 (id 0 is already used
	// by the implicit SCTP outgoing stream built-in the SCTP socket).
	sendStreamId = 123;
	sctpSendStream = sctpSocket.createStream(sendStreamId);

	// Create DataProducer with the corresponding SCTP stream id.
	dataProducer = await transport.produceData(
		{
			sctpStreamParameters :
			{
				streamId : sendStreamId,
				ordered  : true
			}
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

test('ordered DataProducer delivers all messages to the DataConsumer', async () =>
{
	const numMessages = 200;
	let sentMessageBytes = 0;
	let recvMessageBytes = 0;
	let lastSentMessageId = 0;
	let lastRecvMessageId = 0;

	await new Promise((resolve) =>
	{
		const interval = setInterval(() =>
		{
			const id = ++lastSentMessageId;
			const data = Buffer.from(`${id}`);

			// Set ppid of type WebRTC DataChannel string.
			data.ppid = sctp.PPID.WEBRTC_STRING;

			sctpSendStream.write(data);
			sentMessageBytes += data.byteLength;

			if (id === numMessages)
				clearInterval(interval);
		}, 10);

		sctpSocket.on('stream', (stream) =>
		{
			stream.on('data', (data) =>
			{
				recvMessageBytes += data.byteLength;

				const id = Number(data.toString('utf8'));

				if (id === numMessages)
				{
					clearInterval(interval);
					resolve();
				}

				expect(data.ppid).toBe(sctp.PPID.WEBRTC_STRING);
				expect(id).toBe(++lastRecvMessageId);
			});
		});
	});

	expect(lastSentMessageId).toBe(numMessages);
	expect(lastRecvMessageId).toBe(numMessages);
	expect(recvMessageBytes).toBe(sentMessageBytes);

	await expect(dataProducer.getStats())
		.resolves
		.toStrictEqual(
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
		.toStrictEqual(
			[
				{
					type         : 'data-consumer',
					label        : dataConsumer.label,
					protocol     : dataConsumer.protocol,
					messagesSent : numMessages,
					bytesSent    : recvMessageBytes
				}
			]);
}, 8000);
