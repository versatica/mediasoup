import { createSocket } from 'node:dgram';
import {
	SCTP,
	SCTP_STATE,
	WEBRTC_PPID,
	createUdpTransport as createSctpUdpTransport,
	type Transport,
} from 'werift-sctp';
import * as mediasoup from '../';
import { EnhancedEventEmitter, enhancedOnce } from '../enhancedEvents';
import type { WorkerEvents } from '../types';

type TestContext = {
	worker?: mediasoup.types.Worker;
	router?: mediasoup.types.Router;
	plainTransport?: mediasoup.types.PlainTransport;
	dataProducer?: mediasoup.types.DataProducer;
	dataConsumer?: mediasoup.types.DataConsumer;
	sctpClient?: SCTP;
	sctpSendStreamId?: number;
};

const ctx: TestContext = {};

beforeEach(async () => {
	ctx.worker = await mediasoup.createWorker({ disableLiburing: true });

	ctx.router = await ctx.worker.createRouter();

	ctx.plainTransport = await ctx.router.createPlainTransport({
		// https://github.com/nodejs/node/issues/14900.
		listenIp: '127.0.0.1',
		// So we don't need to call plainTransport.connect().
		comedia: true,
		enableSctp: true,
		numSctpStreams: { OS: 256, MIS: 256 },
	});

	ctx.sctpClient = SCTP.client(
		createSctpUdpTransport(createSocket('udp4'), {
			port: ctx.plainTransport.tuple.localPort,
			address: ctx.plainTransport.tuple.localAddress,
		})
	);

	await ctx.sctpClient.start(5000);

	await Promise.race([
		ctx.sctpClient.stateChanged.connected.asPromise(),
		new Promise<void>((resolve, reject) =>
			setTimeout(() => reject(new Error('SCTP connection timeout')), 3000)
		),
	]);

	// Create an explicit SCTP outgoing stream with id 123 (id 0 is already used
	// by the implicit SCTP outgoing stream built-in the SCTP socket).
	ctx.sctpSendStreamId = 123;

	// Create a DataProducer with the corresponding SCTP stream id.
	ctx.dataProducer = await ctx.plainTransport.produceData({
		sctpStreamParameters: {
			streamId: ctx.sctpSendStreamId,
			ordered: true,
		},
		label: 'node-sctp',
		protocol: 'foo & bar 😀😀😀',
	});

	// Create a DataConsumer to receive messages from the DataProducer over the
	// same plainTransport.
	ctx.dataConsumer = await ctx.plainTransport.consumeData({
		dataProducerId: ctx.dataProducer.id,
	});
});

afterEach(async () => {
	// @ts-expect-error
	const upd = ctx.sctpClient?.transport.upd;

	console.log('---- upd:', upd);

	await ctx.sctpClient?.stop();
	// NOTE: SCTP.stop() does not invoke close() on its Transport so
	// `udpSocket.close()` is not called.
	ctx.sctpClient?.transport.close();
	ctx.worker?.close();

	if (ctx.worker?.subprocessClosed === false) {
		await enhancedOnce<WorkerEvents>(ctx.worker, 'subprocessclose');
	}
});

test('SCTP state is connected', async () => {
	expect(ctx.plainTransport!.sctpState == 'connected');
	expect(ctx.sctpClient!.associationState == SCTP_STATE.ESTABLISHED);
}, 2000);

test('ordered DataProducer delivers all SCTP messages to the DataConsumer', async () => {
	const onStream = jest.fn();
	const numMessages = 200;
	let sentMessageBytes = 0;
	let recvMessageBytes = 0;
	let numSentMessages = 0;
	let numReceivedMessages = 0;

	// It must be zero because it's the first DataConsumer on the plainTransport.
	expect(ctx.dataConsumer!.sctpStreamParameters?.streamId).toBe(0);

	await new Promise<void>((resolve, reject) => {
		sendNextMessage();

		function sendNextMessage(): void {
			const id = ++numSentMessages;
			const data = Buffer.from(String(id));
			let ppid: WEBRTC_PPID;

			// Set ppid of type WebRTC DataChannel string.
			if (id < numMessages / 2) {
				ppid = WEBRTC_PPID.STRING;
			}
			// Set ppid of type WebRTC DataChannel binary.
			else {
				ppid = WEBRTC_PPID.BINARY;
			}

			ctx.sctpClient!.send(ctx.sctpSendStreamId!, ppid, data);

			sentMessageBytes += data.byteLength;

			if (id < numMessages) {
				sendNextMessage();
			}
		}

		ctx.sctpClient!.onReceive = (_, __, data) => {
			console.log(data.toString());
		};

		ctx.sctpSocket!.on('stream', onStream);

		// Handle the generated SCTP incoming stream and SCTP messages receives on it.
		ctx.sctpSocket!.on('stream', (stream, streamId) => {
			// It must be zero because it's the first SCTP incoming stream (so first
			// DataConsumer).
			if (streamId !== 0) {
				reject(new Error(`streamId should be 0 but it is ${streamId}`));

				return;
			}

			stream.on('data', (data: Buffer) => {
				++numReceivedMessages;
				recvMessageBytes += data.byteLength;

				const id = Number(data.toString('utf8'));
				// @ts-expect-errors --- sctp library uses `ppid` field.
				const ppid = data.ppid;

				if (id !== numReceivedMessages) {
					reject(
						new Error(
							`id ${id} in message should match numReceivedMessages ${numReceivedMessages}`
						)
					);
				} else if (id === numMessages) {
					resolve();
				} else if (id < numMessages / 2 && ppid !== sctp.PPID.WEBRTC_STRING) {
					reject(
						new Error(
							`ppid in message with id ${id} should be ${sctp.PPID.WEBRTC_STRING} but it is ${ppid}`
						)
					);
				} else if (id > numMessages / 2 && ppid !== sctp.PPID.WEBRTC_BINARY) {
					reject(
						new Error(
							`ppid in message with id ${id} should be ${sctp.PPID.WEBRTC_BINARY} but it is ${ppid}`
						)
					);

					return;
				}
			});
		});
	});

	expect(onStream).toHaveBeenCalledTimes(1);
	expect(numSentMessages).toBe(numMessages);
	expect(numReceivedMessages).toBe(numMessages);
	expect(recvMessageBytes).toBe(sentMessageBytes);

	await expect(ctx.dataProducer!.getStats()).resolves.toMatchObject([
		{
			type: 'data-producer',
			label: ctx.dataProducer!.label,
			protocol: ctx.dataProducer!.protocol,
			messagesReceived: numMessages,
			bytesReceived: sentMessageBytes,
		},
	]);

	await expect(ctx.dataConsumer!.getStats()).resolves.toMatchObject([
		{
			type: 'data-consumer',
			label: ctx.dataConsumer!.label,
			protocol: ctx.dataConsumer!.protocol,
			messagesSent: numMessages,
			bytesSent: recvMessageBytes,
		},
	]);
}, 10000);
