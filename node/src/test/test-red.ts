import * as mediasoup from '../';
import { enhancedOnce } from '../enhancedEvents';
import type { WorkerEvents } from '../types';

type TestContext = {
	mediaCodecs: mediasoup.types.RouterRtpCodecCapability[];
	audioProducerOptions: mediasoup.types.ProducerOptions;
	worker?: mediasoup.types.Worker;
	router?: mediasoup.types.Router;
	directTransport?: mediasoup.types.DirectTransport;
};

const ctx: TestContext = {
	mediaCodecs: [
		{
			kind: 'audio',
			mimeType: 'audio/opus',
			clockRate: 48000,
			channels: 2,
		},
		{
			kind: 'audio',
			mimeType: 'audio/red',
			clockRate: 48000,
			channels: 2,
		},
	],
	audioProducerOptions: {
		kind: 'audio',
		rtpParameters: {
			codecs: [
				{
					mimeType: 'audio/opus',
					payloadType: 111,
					clockRate: 48000,
					channels: 2,
					parameters: {
						useinbandfec: 1,
					},
				},
				{
					mimeType: 'audio/red',
					payloadType: 63,
					clockRate: 48000,
					channels: 2,
				},
			],
			encodings: [{ ssrc: 11111111 }],
		},
	},
};

const OPUS_PAYLOAD_TYPE = 111;
const RED_PAYLOAD_TYPE = 63;

const primaryBlockData = Buffer.from([0x31, 0x32, 0x33, 0x34]);

/**
 * Build a RED payload with two redundant blocks and the primary block, all of
 * them having the given block payload type.
 */
function buildRedPayload(blockPayloadType: number): Buffer {
	return Buffer.concat([
		// Block header 1: F=1, ts offset=1920, block length=3.
		Buffer.from([0x80 | blockPayloadType, 0x1e, 0x00, 0x03]),
		// Block header 2: F=1, ts offset=960, block length=2.
		Buffer.from([0x80 | blockPayloadType, 0x0f, 0x00, 0x02]),
		// Final block header: F=0.
		Buffer.from([blockPayloadType]),
		// Block 1 data.
		Buffer.from([0x11, 0x12, 0x13]),
		// Block 2 data.
		Buffer.from([0x21, 0x22]),
		// Primary block data.
		primaryBlockData,
	]);
}

function buildRtpPacket({
	payloadType,
	seq,
	timestamp,
	ssrc,
	payload,
	extensions = [],
}: {
	payloadType: number;
	seq: number;
	timestamp: number;
	ssrc: number;
	payload: Buffer;
	// One-Byte header extensions.
	extensions?: { id: number; value: Buffer }[];
}): Buffer {
	const header = Buffer.alloc(12);

	header.writeUInt8(extensions.length > 0 ? 0x90 : 0x80, 0);
	header.writeUInt8(payloadType & 0x7f, 1);
	header.writeUInt16BE(seq, 2);
	header.writeUInt32BE(timestamp, 4);
	header.writeUInt32BE(ssrc, 8);

	if (extensions.length === 0) {
		return Buffer.concat([header, payload]);
	}

	let extensionsData = Buffer.concat(
		extensions.map(({ id, value }) =>
			Buffer.concat([Buffer.from([(id << 4) | (value.length - 1)]), value])
		)
	);

	// Pad to 4 bytes.
	extensionsData = Buffer.concat([
		extensionsData,
		Buffer.alloc((4 - (extensionsData.length % 4)) % 4),
	]);

	const extensionHeader = Buffer.alloc(4);

	extensionHeader.writeUInt16BE(0xbede, 0);
	extensionHeader.writeUInt16BE(extensionsData.length / 4, 2);

	return Buffer.concat([header, extensionHeader, extensionsData, payload]);
}

function getPayloadType(rtpPacket: Buffer): number {
	return rtpPacket.readUInt8(1) & 0x7f;
}

function getPayload(rtpPacket: Buffer): Buffer {
	const csrcCount = rtpPacket.readUInt8(0) & 0x0f;
	const hasExtension = (rtpPacket.readUInt8(0) & 0x10) !== 0;
	let offset = 12 + csrcCount * 4;

	if (hasExtension) {
		const extensionLength = rtpPacket.readUInt16BE(offset + 2) * 4;

		offset += 4 + extensionLength;
	}

	return rtpPacket.subarray(offset);
}

beforeEach(async () => {
	ctx.worker = await mediasoup.createWorker();
	ctx.router = await ctx.worker.createRouter({ mediaCodecs: ctx.mediaCodecs });
	ctx.directTransport = await ctx.router.createDirectTransport();
});

afterEach(async () => {
	ctx.worker?.close();

	if (ctx.worker?.subprocessClosed === false) {
		await enhancedOnce<WorkerEvents>(ctx.worker, 'subprocessclose');
	}
});

test('Router RTP capabilities include RED codec', () => {
	const redCodec = ctx.router!.rtpCapabilities.codecs!.find(
		codec => codec.mimeType.toLowerCase() === 'audio/red'
	);

	expect(redCodec).toMatchObject({
		kind: 'audio',
		mimeType: 'audio/red',
		clockRate: 48000,
		channels: 2,
	});
	expect(typeof redCodec!.preferredPayloadType).toBe('number');
}, 2000);

test('transport.consume() keeps RED codec only if supported by the remote endpoint', async () => {
	const audioProducer = await ctx.directTransport!.produce(
		ctx.audioProducerOptions
	);

	const routerRtpCapabilities = ctx.router!.rtpCapabilities;
	const routerOpusCodec = routerRtpCapabilities.codecs!.find(
		codec => codec.mimeType.toLowerCase() === 'audio/opus'
	)!;
	const routerRedCodec = routerRtpCapabilities.codecs!.find(
		codec => codec.mimeType.toLowerCase() === 'audio/red'
	)!;

	const redConsumer = await ctx.directTransport!.consume({
		producerId: audioProducer.id,
		rtpCapabilities: routerRtpCapabilities,
	});

	expect(redConsumer.rtpParameters.codecs).toEqual([
		expect.objectContaining({
			mimeType: 'audio/opus',
			payloadType: routerOpusCodec.preferredPayloadType,
			parameters: { useinbandfec: 1 },
		}),
		expect.objectContaining({
			mimeType: 'audio/red',
			payloadType: routerRedCodec.preferredPayloadType,
			parameters: {},
		}),
	]);

	const nonRedConsumer = await ctx.directTransport!.consume({
		producerId: audioProducer.id,
		rtpCapabilities: {
			codecs: routerRtpCapabilities.codecs!.filter(
				codec => codec.mimeType.toLowerCase() !== 'audio/red'
			),
			headerExtensions: routerRtpCapabilities.headerExtensions,
		},
	});

	expect(nonRedConsumer.rtpParameters.codecs).toEqual([
		expect.objectContaining({
			mimeType: 'audio/opus',
			payloadType: routerOpusCodec.preferredPayloadType,
		}),
	]);
}, 2000);

test('router.canConsume() returns false if the remote endpoint just supports RED', async () => {
	const audioProducer = await ctx.directTransport!.produce(
		ctx.audioProducerOptions
	);

	expect(
		ctx.router!.canConsume({
			producerId: audioProducer.id,
			rtpCapabilities: {
				codecs: ctx.router!.rtpCapabilities.codecs!.filter(
					codec => codec.mimeType.toLowerCase() === 'audio/red'
				),
			},
		})
	).toBe(false);
}, 2000);

test('RED packets are forwarded to Consumers supporting RED and unwrapped for others', async () => {
	const audioProducer = await ctx.directTransport!.produce(
		ctx.audioProducerOptions
	);

	const routerRtpCapabilities = ctx.router!.rtpCapabilities;
	const routerOpusPayloadType = routerRtpCapabilities.codecs!.find(
		codec => codec.mimeType.toLowerCase() === 'audio/opus'
	)!.preferredPayloadType!;
	const routerRedPayloadType = routerRtpCapabilities.codecs!.find(
		codec => codec.mimeType.toLowerCase() === 'audio/red'
	)!.preferredPayloadType!;

	// Ensure that payload types are really mapped.
	expect(routerOpusPayloadType).not.toBe(OPUS_PAYLOAD_TYPE);
	expect(routerRedPayloadType).not.toBe(RED_PAYLOAD_TYPE);

	const redConsumer = await ctx.directTransport!.consume({
		producerId: audioProducer.id,
		rtpCapabilities: routerRtpCapabilities,
	});

	const nonRedConsumer = await ctx.directTransport!.consume({
		producerId: audioProducer.id,
		rtpCapabilities: {
			codecs: routerRtpCapabilities.codecs!.filter(
				codec => codec.mimeType.toLowerCase() !== 'audio/red'
			),
			headerExtensions: routerRtpCapabilities.headerExtensions,
		},
	});

	const redConsumerPacketsPromise: Promise<Buffer[]> = new Promise(resolve => {
		const packets: Buffer[] = [];

		redConsumer.on('rtp', (packet: Buffer) => {
			packets.push(packet);

			if (packets.length === 2) {
				resolve(packets);
			}
		});
	});

	const nonRedConsumerPacketsPromise: Promise<Buffer[]> = new Promise(
		resolve => {
			const packets: Buffer[] = [];

			nonRedConsumer.on('rtp', (packet: Buffer) => {
				packets.push(packet);

				if (packets.length === 2) {
					resolve(packets);
				}
			});
		}
	);

	const ssrc = ctx.audioProducerOptions.rtpParameters.encodings![0]!.ssrc!;

	// Send a RED packet.
	audioProducer.send(
		buildRtpPacket({
			payloadType: RED_PAYLOAD_TYPE,
			seq: 1000,
			timestamp: 960000,
			ssrc,
			payload: buildRedPayload(OPUS_PAYLOAD_TYPE),
		})
	);

	// Send a regular Opus packet.
	audioProducer.send(
		buildRtpPacket({
			payloadType: OPUS_PAYLOAD_TYPE,
			seq: 1001,
			timestamp: 960960,
			ssrc,
			payload: primaryBlockData,
		})
	);

	const [redConsumerPackets, nonRedConsumerPackets] = await Promise.all([
		redConsumerPacketsPromise,
		nonRedConsumerPacketsPromise,
	]);

	// The Consumer supporting RED gets the RED packet with mapped payload types
	// (also in its blocks) and the Opus packet.
	expect(getPayloadType(redConsumerPackets[0]!)).toBe(routerRedPayloadType);
	expect(getPayload(redConsumerPackets[0]!)).toEqual(
		buildRedPayload(routerOpusPayloadType)
	);
	expect(getPayloadType(redConsumerPackets[1]!)).toBe(routerOpusPayloadType);
	expect(getPayload(redConsumerPackets[1]!)).toEqual(primaryBlockData);

	// The Consumer not supporting RED gets the primary block of the RED packet
	// as a regular Opus packet and the Opus packet.
	expect(getPayloadType(nonRedConsumerPackets[0]!)).toBe(routerOpusPayloadType);
	expect(getPayload(nonRedConsumerPackets[0]!)).toEqual(primaryBlockData);
	expect(getPayloadType(nonRedConsumerPackets[1]!)).toBe(routerOpusPayloadType);
	expect(getPayload(nonRedConsumerPackets[1]!)).toEqual(primaryBlockData);

	// Sequence numbers must be consecutive in both Consumers.
	for (const packets of [redConsumerPackets, nonRedConsumerPackets]) {
		expect((packets[0]!.readUInt16BE(2) + 1) & 0xffff).toBe(
			packets[1]!.readUInt16BE(2)
		);
	}
}, 4000);

test('consumer.rtpParameters.codecs[0] is the media codec even if the Producer lists RED first', async () => {
	const audioProducer = await ctx.directTransport!.produce({
		kind: 'audio',
		rtpParameters: {
			codecs: [...ctx.audioProducerOptions.rtpParameters.codecs].reverse(),
			encodings: [{ ssrc: 22222222 }],
		},
	});

	const consumer = await ctx.directTransport!.consume({
		producerId: audioProducer.id,
		rtpCapabilities: ctx.router!.rtpCapabilities,
	});

	expect(consumer.rtpParameters.codecs.map(codec => codec.mimeType)).toEqual([
		'audio/opus',
		'audio/red',
	]);
}, 2000);

test('RED packets create the Producer RTP stream when there is no signaled SSRC', async () => {
	for (const { mid, encoding, extensions } of [
		// Single encoding without SSRC nor RID.
		{
			mid: 'AUDIO1',
			encoding: {},
			extensions: [{ id: 1, value: Buffer.from('AUDIO1') }],
		},
		// Single encoding with RID.
		{
			mid: 'AUDIO2',
			encoding: { rid: 'r0' },
			extensions: [
				{ id: 1, value: Buffer.from('AUDIO2') },
				{ id: 2, value: Buffer.from('r0') },
			],
		},
	]) {
		const audioProducer = await ctx.directTransport!.produce({
			kind: 'audio',
			rtpParameters: {
				mid,
				codecs: ctx.audioProducerOptions.rtpParameters.codecs,
				headerExtensions: [
					{ uri: 'urn:ietf:params:rtp-hdrext:sdes:mid', id: 1 },
					{ uri: 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id', id: 2 },
				],
				encodings: [encoding],
			},
		});

		const consumer = await ctx.directTransport!.consume({
			producerId: audioProducer.id,
			rtpCapabilities: ctx.router!.rtpCapabilities,
		});

		const packetPromise: Promise<Buffer> = new Promise(resolve =>
			consumer.once('rtp', resolve)
		);

		// Just send RED packets, as endpoints may do.
		audioProducer.send(
			buildRtpPacket({
				payloadType: RED_PAYLOAD_TYPE,
				seq: 1000,
				timestamp: 960000,
				ssrc: 33333333,
				payload: buildRedPayload(OPUS_PAYLOAD_TYPE),
				extensions,
			})
		);

		const packet = await packetPromise;
		const routerRedPayloadType = ctx.router!.rtpCapabilities.codecs!.find(
			codec => codec.mimeType.toLowerCase() === 'audio/red'
		)!.preferredPayloadType!;

		expect(getPayloadType(packet)).toBe(routerRedPayloadType);

		audioProducer.close();
	}
}, 4000);

test('invalid RED packets are discarded', async () => {
	const audioProducer = await ctx.directTransport!.produce(
		ctx.audioProducerOptions
	);

	const routerOpusPayloadType = ctx.router!.rtpCapabilities.codecs!.find(
		codec => codec.mimeType.toLowerCase() === 'audio/opus'
	)!.preferredPayloadType!;

	const consumer = await ctx.directTransport!.consume({
		producerId: audioProducer.id,
		rtpCapabilities: ctx.router!.rtpCapabilities,
	});

	const packetPromise: Promise<Buffer> = new Promise(resolve =>
		consumer.once('rtp', resolve)
	);

	const ssrc = ctx.audioProducerOptions.rtpParameters.encodings![0]!.ssrc!;

	// RED packet with a block payload type unknown to the Producer.
	audioProducer.send(
		buildRtpPacket({
			payloadType: RED_PAYLOAD_TYPE,
			seq: 1000,
			timestamp: 960000,
			ssrc,
			payload: buildRedPayload(5),
		})
	);

	// RED packet with a truncated block header.
	audioProducer.send(
		buildRtpPacket({
			payloadType: RED_PAYLOAD_TYPE,
			seq: 1001,
			timestamp: 960960,
			ssrc,
			payload: Buffer.from([0x80 | OPUS_PAYLOAD_TYPE, 0x0f]),
		})
	);

	// Valid Opus packet.
	audioProducer.send(
		buildRtpPacket({
			payloadType: OPUS_PAYLOAD_TYPE,
			seq: 1002,
			timestamp: 961920,
			ssrc,
			payload: primaryBlockData,
		})
	);

	// The first packet received by the Consumer must be the Opus one.
	const packet = await packetPromise;

	expect(getPayloadType(packet)).toBe(routerOpusPayloadType);
	expect(getPayload(packet)).toEqual(primaryBlockData);
}, 4000);

test('RED packets whose primary block is DTX are discarded if ignoreDtx is set', async () => {
	const audioProducer = await ctx.directTransport!.produce(
		ctx.audioProducerOptions
	);

	const routerRtpCapabilities = ctx.router!.rtpCapabilities;
	const redConsumer = await ctx.directTransport!.consume({
		producerId: audioProducer.id,
		rtpCapabilities: routerRtpCapabilities,
		ignoreDtx: true,
	});
	const nonRedConsumer = await ctx.directTransport!.consume({
		producerId: audioProducer.id,
		rtpCapabilities: {
			codecs: routerRtpCapabilities.codecs!.filter(
				codec => codec.mimeType.toLowerCase() !== 'audio/red'
			),
			headerExtensions: routerRtpCapabilities.headerExtensions,
		},
		ignoreDtx: true,
	});

	const redConsumerPacketPromise: Promise<Buffer> = new Promise(resolve =>
		redConsumer.once('rtp', resolve)
	);
	const nonRedConsumerPacketPromise: Promise<Buffer> = new Promise(resolve =>
		nonRedConsumer.once('rtp', resolve)
	);

	const ssrc = ctx.audioProducerOptions.rtpParameters.encodings![0]!.ssrc!;
	// Opus code 0 TOC byte only, which means DTX.
	const dtxPrimaryBlockData = Buffer.from([0x78]);

	// RED packet whose primary block is DTX.
	audioProducer.send(
		buildRtpPacket({
			payloadType: RED_PAYLOAD_TYPE,
			seq: 1000,
			timestamp: 960000,
			ssrc,
			payload: Buffer.concat([
				Buffer.from([0x80 | OPUS_PAYLOAD_TYPE, 0x0f, 0x00, 0x04]),
				Buffer.from([OPUS_PAYLOAD_TYPE]),
				primaryBlockData,
				dtxPrimaryBlockData,
			]),
		})
	);

	// Regular RED packet.
	audioProducer.send(
		buildRtpPacket({
			payloadType: RED_PAYLOAD_TYPE,
			seq: 1001,
			timestamp: 960960,
			ssrc,
			payload: buildRedPayload(OPUS_PAYLOAD_TYPE),
		})
	);

	const [redConsumerPacket, nonRedConsumerPacket] = await Promise.all([
		redConsumerPacketPromise,
		nonRedConsumerPacketPromise,
	]);

	// The first packet received by both Consumers must be the regular one.
	expect(getPayload(redConsumerPacket).length).toBe(
		buildRedPayload(OPUS_PAYLOAD_TYPE).length
	);
	expect(getPayload(nonRedConsumerPacket)).toEqual(primaryBlockData);
}, 4000);
