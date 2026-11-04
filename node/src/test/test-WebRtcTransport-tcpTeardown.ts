import { createHmac, randomBytes } from 'node:crypto';
import { once } from 'node:events';
import { createConnection, type Socket } from 'node:net';
import type * as mediasoup from '../';
import { enhancedOnce } from '../enhancedEvents';
import type { WorkerEvents, WebRtcTransportEvents } from '../types';
import { WorkerImpl } from '../Worker';

test('TCP ICE teardown keeps the worker usable', async () => {
	let worker: mediasoup.types.Worker | undefined;
	let router: mediasoup.types.Router;

	const sockets: Set<Socket> = new Set();
	const transports: Set<mediasoup.types.WebRtcTransport> = new Set();
	const cancellation = new AbortController();
	const cleanupCancellation = new AbortController();
	const { signal } = cancellation;
	let exerciseTimer: NodeJS.Timeout | undefined;
	let totalTimer: NodeJS.Timeout | undefined;

	function cleanup(): void {
		for (const socket of sockets) {
			socket.destroy();
		}
		for (const transport of transports) {
			transport.close();
		}
		worker?.close();
	}

	function crc32(bytes: Buffer): number {
		let crc = 0xffffffff;

		for (const byte of bytes) {
			crc ^= byte;
			for (let bit = 0; bit < 8; ++bit) {
				crc = (crc >>> 1) ^ (crc & 1 ? 0xedb88320 : 0);
			}
		}

		return (crc ^ 0xffffffff) >>> 0;
	}

	function bindingRequest(ice: mediasoup.types.IceParameters): Buffer {
		const username = Buffer.from(`${ice.usernameFragment}:synthetic`);
		const usernameAttr = Buffer.alloc(4 + Math.ceil(username.length / 4) * 4);

		usernameAttr.writeUInt16BE(0x0006, 0);
		usernameAttr.writeUInt16BE(username.length, 2);
		username.copy(usernameAttr, 4);

		const controlling = Buffer.alloc(12);

		controlling.writeUInt16BE(0x802a, 0);
		controlling.writeUInt16BE(8, 2);
		randomBytes(8).copy(controlling, 4);

		const attributes = Buffer.concat([
			usernameAttr,
			Buffer.from('002400046e0001ff', 'hex'), // PRIORITY.
			controlling,
			Buffer.from('00250000', 'hex'), // USE-CANDIDATE.
		]);
		const header = Buffer.alloc(20);

		header.writeUInt16BE(0x0001, 0);
		header.writeUInt16BE(attributes.length + 24, 2);
		header.writeUInt32BE(0x2112a442, 4);
		randomBytes(12).copy(header, 8);

		const integrity = Buffer.alloc(24);

		integrity.writeUInt16BE(0x0008, 0);
		integrity.writeUInt16BE(20, 2);
		createHmac('sha1', ice.password)
			.update(Buffer.concat([header, attributes]))
			.digest()
			.copy(integrity, 4);
		header.writeUInt16BE(attributes.length + 32, 2);

		const beforeFingerprint = Buffer.concat([header, attributes, integrity]);
		const fingerprint = Buffer.alloc(8);

		fingerprint.writeUInt16BE(0x8028, 0);
		fingerprint.writeUInt16BE(4, 2);
		fingerprint.writeUInt32BE((crc32(beforeFingerprint) ^ 0x5354554e) >>> 0, 4);

		return Buffer.concat([beforeFingerprint, fingerprint]);
	}

	async function tcpConnection(): Promise<{
		transport: mediasoup.types.WebRtcTransport;
		socket: Socket;
	}> {
		signal.throwIfAborted();

		const transport = await router.createWebRtcTransport({
			listenInfos: [{ protocol: 'tcp', ip: '127.0.0.1' }],
		});

		transports.add(transport);
		signal.throwIfAborted();

		const socket = createConnection({
			host: '127.0.0.1',
			port: transport.iceCandidates[0]!.port,
		});

		sockets.add(socket);

		const request = bindingRequest(transport.iceParameters);
		const nominated = enhancedOnce<WebRtcTransportEvents>(
			transport,
			'icestatechange',
			{ signal }
		);
		const response: Promise<Buffer> = new Promise((resolve, reject) => {
			let pending: Buffer = Buffer.alloc(0);

			function stopListening(): void {
				socket.off('data', onData);
				socket.off('close', onClose);
				socket.off('error', onError);
				socket.off('connect', onConnect);
				signal.removeEventListener('abort', onAbort);
			}

			const onAbort = (): void => {
				stopListening();
				reject(
					signal.reason instanceof Error
						? signal.reason
						: new Error('TCP ICE teardown aborted')
				);
			};
			const onClose = (): void => {
				stopListening();
				reject(new Error('TCP closed before STUN reply'));
			};
			const onError = (error: Error): void => {
				stopListening();
				reject(error);
			};
			const onData = (chunk: Buffer): void => {
				pending = Buffer.concat([pending, chunk]);
				if (
					pending.length < 2 ||
					pending.length < pending.readUInt16BE(0) + 2
				) {
					return;
				}
				stopListening();
				resolve(pending.subarray(2));
			};
			const onConnect = (): void => {
				const frame = Buffer.alloc(2 + request.length);

				frame.writeUInt16BE(request.length, 0);
				request.copy(frame, 2);
				socket.write(frame);
			};

			socket.on('data', onData);
			socket.once('close', onClose);
			socket.once('error', onError);
			socket.once('connect', onConnect);
			signal.addEventListener('abort', onAbort, { once: true });
		});

		const [reply] = await Promise.all([response, nominated]);

		expect(reply.length).toBeGreaterThanOrEqual(20);
		expect(reply.readUInt16BE(0)).toBe(0x0101);
		expect(reply.subarray(4, 20)).toEqual(request.subarray(4, 20));
		expect(transport.iceState).toBe('completed');
		expect(transport.iceSelectedTuple?.protocol).toBe('tcp');

		return { transport, socket };
	}

	async function exercise(): Promise<void> {
		worker = new WorkerImpl({ logLevel: 'error' });
		await Promise.race([
			enhancedOnce<WorkerEvents>(worker, '@success', { signal }),
			enhancedOnce<WorkerEvents>(worker, '@failure', { signal }).then(
				([error]) => {
					throw error;
				}
			),
		]);
		router = await worker.createRouter();
		signal.throwIfAborted();

		const active = await tcpConnection();

		expect(active.socket.destroyed).toBe(false);

		const closed = once(active.socket, 'close', { signal });

		active.transport.close();
		await Promise.all([worker!.dump(), closed]);

		const ordinary = await tcpConnection();
		const disconnected = enhancedOnce<WebRtcTransportEvents>(
			ordinary.transport,
			'icestatechange',
			{ signal }
		);

		ordinary.socket.destroy();
		await disconnected;
		expect(ordinary.transport.iceState).toBe('disconnected');
		ordinary.transport.close();
		await worker!.dump();

		const both = await router.createWebRtcTransport({
			listenInfos: [
				{ protocol: 'tcp', ip: '127.0.0.1' },
				{ protocol: 'udp', ip: '127.0.0.1' },
			],
		});

		transports.add(both);
		expect(
			both.iceCandidates.map(candidate => candidate.protocol).sort()
		).toEqual(['tcp', 'udp']);
		both.close();
		await worker!.dump();
	}

	const deadline: Promise<never> = new Promise((_, reject) => {
		exerciseTimer = setTimeout(() => {
			const error = new Error('TCP ICE teardown deadline exceeded');

			cancellation.abort(error);
			reject(error);
		}, 9000);
	});
	const totalDeadline: Promise<never> = new Promise((_, reject) => {
		totalTimer = setTimeout(() => {
			cleanupCancellation.abort();
			reject(new Error('TCP ICE teardown cleanup deadline exceeded'));
		}, 12000);
	});
	const work = exercise();

	try {
		await Promise.race([work, deadline, totalDeadline]);
	} finally {
		clearTimeout(exerciseTimer);
		cancellation.abort();

		const workerClosed =
			!worker || worker.subprocessClosed
				? Promise.resolve()
				: enhancedOnce<WorkerEvents>(worker, 'subprocessclose', {
						signal: cleanupCancellation.signal,
					});

		try {
			cleanup();
			await Promise.race([
				Promise.all([work.catch(() => {}), workerClosed]),
				totalDeadline,
			]);
		} finally {
			clearTimeout(totalTimer);
			cleanupCancellation.abort();
		}
	}
}, 15000);
