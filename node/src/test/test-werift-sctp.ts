import { createSocket } from 'node:dgram';
import {
	SCTP,
	SCTP_STATE,
	WEBRTC_PPID,
	createUdpTransport as createSctpUdpTransport,
} from 'werift-sctp';
import * as mediasoup from '../';
import { enhancedOnce } from '../enhancedEvents';
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
	ctx.worker = await mediasoup.createWorker({
		disableLiburing: true,
	});

	ctx.router = await ctx.worker.createRouter();

	ctx.plainTransport = await ctx.router.createPlainTransport({
		// https://github.com/nodejs/node/issues/14900.
		listenIp: '127.0.0.1',
		// So we don't need to call plainTransport.connect().
		comedia: true,
		enableSctp: true,
		numSctpStreams: { OS: 256, MIS: 256 },
	});

	// Create an explicit SCTP outgoing stream id.
	ctx.sctpSendStreamId = 123;

	ctx.sctpClient = SCTP.client(
		createSctpUdpTransport(createSocket('udp4'), {
			port: ctx.plainTransport.tuple.localPort,
			address: ctx.plainTransport.tuple.localAddress,
		})
	);

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

	let connectionTimeoutTimer: NodeJS.Timeout | undefined;

	await Promise.race([
		// Wait for SCTP to become connected in both the PlainTransport and in the
		// werift-sctp client.
		Promise.all([
			// Connect werift-sctp client (this resolves once SCTP is connected).
			ctx.sctpClient.start(5000),
			// This resolves once connected too.
			ctx.sctpClient.stateChanged.connected.asPromise(),
			// Wait for SCTP state in the mediasoup PlainTransport to be "connected".
			new Promise<void>((resolve, reject) => {
				if (ctx.plainTransport?.sctpState === 'connected') {
					resolve();
				} else {
					ctx.plainTransport?.on('sctpstatechange', state => {
						if (state === 'connected') {
							resolve();
						} else if (state === 'failed' || state === 'closed') {
							reject(
								new Error(
									'SCTP connection in PlainTransport failed or was closed'
								)
							);
						}
					});
				}
			}),
		]),
		new Promise<void>((resolve, reject) => {
			connectionTimeoutTimer = setTimeout(
				() => reject(new Error('SCTP connection timeout')),
				3000
			);
		}),
	]);

	clearTimeout(connectionTimeoutTimer);
}, 5000);

afterEach(async () => {
	await ctx.sctpClient?.stop();
	ctx.sctpClient?.transport.close();
	ctx.worker?.close();

	if (ctx.worker?.subprocessClosed === false) {
		await enhancedOnce<WorkerEvents>(ctx.worker, 'subprocessclose');
	}
});

test('SCTP state is connected', () => {
	expect(ctx.plainTransport!.sctpState).toBe('connected');
	expect(ctx.sctpClient!.associationState).toBe(SCTP_STATE.ESTABLISHED);
});

test('ordered DataProducer delivers all SCTP messages to the DataConsumer', async () => {
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

			void ctx.sctpClient!.send(ctx.sctpSendStreamId!, ppid, data);

			sentMessageBytes += data.byteLength;

			if (id < numMessages) {
				sendNextMessage();
			}
		}

		ctx.sctpClient!.onReceive.subscribe(
			(streamId: number, ppid: WEBRTC_PPID, data: Buffer) => {
				// `streamId`  must be zero because it's the first SCTP incoming stream
				// (so first DataConsumer).
				if (streamId !== 0) {
					reject(new Error(`streamId should be 0 but it is ${streamId}`));

					return;
				}

				++numReceivedMessages;
				recvMessageBytes += data.byteLength;

				const id = Number(data.toString('utf8'));

				if (id !== numReceivedMessages) {
					reject(
						new Error(
							`id ${id} in message should match numReceivedMessages ${numReceivedMessages}`
						)
					);
				} else if (id === numMessages) {
					resolve();
				} else if (id < numMessages / 2 && ppid !== WEBRTC_PPID.STRING) {
					reject(
						new Error(
							`ppid in message with id ${id} should be ${WEBRTC_PPID.STRING} but it is ${ppid}`
						)
					);
				} else if (id > numMessages / 2 && ppid !== WEBRTC_PPID.BINARY) {
					reject(
						new Error(
							`ppid in message with id ${id} should be ${WEBRTC_PPID.BINARY} but it is ${ppid}`
						)
					);

					return;
				}
			}
		);
	});

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
